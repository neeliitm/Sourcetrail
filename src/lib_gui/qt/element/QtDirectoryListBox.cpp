#include "qt/element/QtDirectoryListBox.h"

#include <QBoxLayout>
#include <QMimeData>
#include <QScrollBar>
#include <QTimer>

#include "qt/element/QtIconButton.h"
#include "qt/utility/utilityQt.h"
#include "qt/utility/QtFileDialog.h"
#include "qt/window/QtTextEditDialog.h"
#include "utility/ResourcePaths.h"
#include "utility/utilityString.h"

QtListItemWidget::QtListItemWidget(QtDirectoryListBox* list, QListWidgetItem* item, QWidget *parent)
	: QWidget(parent)
	, m_list(list)
	, m_item(item)
{
	QBoxLayout* layout = new QHBoxLayout();
	layout->setSpacing(3);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setAlignment(Qt::AlignTop);

	m_data = new QtLineEdit(this);
	m_data->setAttribute(Qt::WA_MacShowFocusRect, 0);
	m_data->setAttribute(Qt::WA_LayoutUsesWidgetRect); // fixes layouting on Mac
	m_data->setObjectName("field");
    m_data->setAcceptDrops(false);

	m_button = new QtIconButton(
		(ResourcePaths::getGuiPath().str() + "window/dots.png").c_str(),
		(ResourcePaths::getGuiPath().str() + "window/dots_hover.png").c_str());
	m_button->setIconSize(QSize(16, 16));
	m_button->setObjectName("dotsButton");

	layout->addWidget(m_data);
	layout->addWidget(m_button);

	if (list->isForStrings())
	{
		m_button->hide();
	}

	setLayout(layout);

	connect(m_button, &QPushButton::clicked, this, &QtListItemWidget::handleButtonPress);
	connect(m_data, &QtLineEdit::focus, this, &QtListItemWidget::handleFocus);
}

QString QtListItemWidget::getText()
{
	return m_data->text();
}

void QtListItemWidget::setText(QString text)
{
	FilePath relativeRoot = m_list->getRelativeRootDirectory();
	if (!relativeRoot.empty())
	{
		FilePath path(text.toStdString());
		FilePath relPath(path.relativeTo(relativeRoot));
		if (relPath.str().size() < path.str().size())
		{
			text = QString::fromStdString(relPath.str());
		}
	}

	m_data->setText(text);
}

void QtListItemWidget::setFocus()
{
    m_data->setFocus(Qt::OtherFocusReason);
}

void QtListItemWidget::handleButtonPress()
{
	FilePath path(m_data->text().toStdString());
	FilePath relativeRoot = m_list->getRelativeRootDirectory();
	if (!path.empty() && !path.isAbsolute() && !relativeRoot.empty())
	{
		path = relativeRoot.concat(path);
	}

	QStringList list = QtFileDialog::getFileNamesAndDirectories(this, QString::fromStdString(path.str()));
	if (!list.isEmpty())
	{
		setText(list.at(0));
	}

	for (int i = 1; i < list.size(); i++)
	{
		m_list->addListBoxItemWithText(list.at(i));
	}

	handleFocus();
}

void QtListItemWidget::handleFocus()
{
	m_list->selectItem(m_item);
}


QtDirectoryListBox::QtDirectoryListBox(QWidget *parent, const QString& listName, bool forStrings)
	: QFrame(parent)
	, m_listName(listName)
	, m_forStrings(forStrings)
{
	QBoxLayout* layout = new QVBoxLayout();
	layout->setSpacing(0);
	layout->setContentsMargins(0, 6, 0, 0);
	layout->setAlignment(Qt::AlignTop);

	m_list = new QListWidget(this);
	m_list->setObjectName("list");
	m_list->setAttribute(Qt::WA_MacShowFocusRect, 0);
	m_list->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	setStyleSheet(utility::getStyleSheet(ResourcePaths::getGuiPath().concat(FilePath("window/listbox.css"))).c_str());
	layout->addWidget(m_list);

	QWidget* buttonContainer = new QWidget(this);
	buttonContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	buttonContainer->setObjectName("bar");

	QHBoxLayout* innerLayout = new QHBoxLayout();
	innerLayout->setContentsMargins(8, 4, 8, 2);
	innerLayout->setSpacing(0);

	m_addButton = new QtIconButton(
		(ResourcePaths::getGuiPath().str() + "window/plus.png").c_str(),
		(ResourcePaths::getGuiPath().str() + "window/plus_hover.png").c_str());
	m_addButton->setIconSize(QSize(16, 16));
	m_addButton->setObjectName("plusButton");
	m_addButton->setToolTip("add line");
	innerLayout->addWidget(m_addButton);

	m_removeButton = new QtIconButton(
		(ResourcePaths::getGuiPath().str() + "window/minus.png").c_str(),
		(ResourcePaths::getGuiPath().str() + "window/minus_hover.png").c_str());
	m_removeButton->setIconSize(QSize(16, 16));
	m_removeButton->setObjectName("minusButton");
	m_removeButton->setToolTip("remove line");
	innerLayout->addWidget(m_removeButton);

	innerLayout->addStretch();

	QLabel* dropInfoText = new QLabel("Drop Files & Folders");
	dropInfoText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	dropInfoText->setObjectName("dropInfo");
	dropInfoText->setAlignment(Qt::AlignRight);
	innerLayout->addWidget(dropInfoText);

	QPushButton* editButton = new QtIconButton(
		(ResourcePaths::getGuiPath().str() + "code_view/images/edit.png").c_str(),
		QString());
	editButton->setIconSize(QSize(16, 16));
	editButton->setObjectName("editButton");
	editButton->setToolTip("edit plain text");
	innerLayout->addWidget(editButton);

	if (isForStrings())
	{
		dropInfoText->hide();
	}

	layout->addStretch();

	buttonContainer->setLayout(innerLayout);
	layout->addWidget(buttonContainer);
	setLayout(layout);

	connect(m_addButton, &QPushButton::clicked, this, &QtDirectoryListBox::addListBoxItem);
	connect(m_removeButton, &QPushButton::clicked, this, &QtDirectoryListBox::removeListBoxItem);
	connect(editButton, &QPushButton::clicked, this, &QtDirectoryListBox::showEditDialog);

	setAcceptDrops(true);
	setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Minimum);
	setMaximumHeight(200);
	resize();
}

QSize QtDirectoryListBox::sizeHint() const
{
	return QSize(QFrame::sizeHint().width(), 100);
}

void QtDirectoryListBox::clear()
{
	m_list->clear();
}

bool QtDirectoryListBox::event(QEvent* event)
{
	// Prevent nested ScrollAreas to scroll at the same time;
	if (event->type() == QEvent::Wheel)
	{
		QRect rect = m_list->viewport()->rect();
		QPoint pos = m_list->mapFromGlobal( dynamic_cast<QWheelEvent*>(event)->globalPos());
		QScrollBar* bar = m_list->verticalScrollBar();

		if (bar->minimum() != bar->maximum() && rect.contains(pos))
		{
			bool down = dynamic_cast<QWheelEvent*>(event)->angleDelta().y() < 0;

			if ((down && bar->value() != bar->maximum()) || (!down && bar->value() != bar->minimum()))
			{
				return true;
			}
		}
	}

	return QFrame::event(event);
}

void QtDirectoryListBox::dropEvent(QDropEvent *event)
{
	foreach(QUrl url, event->mimeData()->urls())
	{
		QtListItemWidget* widget = addListBoxItem();
		widget->setText(url.toLocalFile());
	}
}

std::vector<FilePath> QtDirectoryListBox::getList()
{
	std::vector<std::string> strList = getStringList();
	std::vector<FilePath> list;
	for (const std::string& str : strList)
	{
		list.push_back(FilePath(str));
	}
	return list;
}

void QtDirectoryListBox::setList(const std::vector<FilePath>& list)
{
	std::vector<std::string> strList;
	for (const FilePath& path : list)
	{
		strList.push_back(path.str());
	}
	setStringList(strList);
}

std::vector<std::string> QtDirectoryListBox::getStringList()
{
	std::vector<std::string> list;
	for (int i = 0; i < m_list->count(); ++i)
	{
		QtListItemWidget* widget = dynamic_cast<QtListItemWidget*>(m_list->itemWidget(m_list->item(i)));
		list.push_back(widget->getText().toStdString());
	}
	return list;
}

void QtDirectoryListBox::setStringList(const std::vector<std::string>& list)
{
	clear();

	FilePath root = m_relativeRootDirectory;
	m_relativeRootDirectory = FilePath();

	for (const std::string& str : list)
	{
		addListBoxItemWithText(QString::fromStdString(str));
	}

	m_relativeRootDirectory = root;

	QTimer::singleShot(1, this, &QtDirectoryListBox::resize);
}

void QtDirectoryListBox::addListBoxItemWithText(const QString& text)
{
	QtListItemWidget* widget = addListBoxItem();
	widget->setText(text);
}

void QtDirectoryListBox::selectItem(QListWidgetItem* item)
{
	for (int i = 0; i < m_list->count(); i++)
	{
		m_list->item(i)->setSelected(false);
	}

	item->setSelected(true);
}

bool QtDirectoryListBox::isForStrings() const
{
	return m_forStrings;
}

const FilePath& QtDirectoryListBox::getRelativeRootDirectory() const
{
	return m_relativeRootDirectory;
}

void QtDirectoryListBox::setRelativeRootDirectory(const FilePath& dir)
{
	m_relativeRootDirectory = dir;
}

void QtDirectoryListBox::resize()
{
	int height = m_list->height() - m_list->viewport()->height();

	if (m_list->count() > 0)
	{
		height += (m_list->itemWidget(m_list->item(0))->height() + 1) * m_list->count() + 7;
	}

	if (height < 0)
	{
		height = 0;
	}

	m_list->setMaximumHeight(height);
}

QtListItemWidget* QtDirectoryListBox::addListBoxItem()
{
	QListWidgetItem *item = new QListWidgetItem(m_list);
	m_list->addItem(item);

	QtListItemWidget* widget = new QtListItemWidget(this, item);
	m_list->setItemWidget(item, widget);

	widget->setFocus();

	resize();

	return widget;
}

void QtDirectoryListBox::removeListBoxItem()
{
	if (!m_list->selectedItems().size())
	{
		return;
	}

	int rowIndex = m_list->row(m_list->selectedItems().first());

	qDeleteAll(m_list->selectedItems());

	if (rowIndex == m_list->count())
	{
		rowIndex -= 1;
	}

	if (rowIndex >= 0)
	{
		m_list->setCurrentRow(rowIndex);
	}

	resize();
}

void QtDirectoryListBox::dragEnterEvent(QDragEnterEvent *event)
{
	event->accept();
}

void QtDirectoryListBox::showEditDialog()
{
	if (!m_editDialog)
	{
		m_editDialog = std::make_shared<QtTextEditDialog>(m_listName, "Edit the list in plain text. Each line is one item.");
		m_editDialog->setup();

		m_editDialog->setText(utility::join(getStringList(), "\n"));

		connect(m_editDialog.get(), &QtTextEditDialog::canceled, this, &QtDirectoryListBox::canceledEditDialog);
		connect(m_editDialog.get(), &QtTextEditDialog::finished, this, &QtDirectoryListBox::savedEditDialog);
	}

	m_editDialog->showWindow();
	m_editDialog->raise();
}

void QtDirectoryListBox::canceledEditDialog()
{
	m_editDialog->hide();
	m_editDialog.reset();

	window()->raise();
}

void QtDirectoryListBox::savedEditDialog()
{
	std::vector<std::string> lines = utility::splitToVector(m_editDialog->getText(), "\n");
	for (size_t i = 0; i < lines.size(); i++)
	{
		lines[i] = utility::trim(lines[i]);

		if (!lines[i].size())
		{
			lines.erase(lines.begin() + i);
			i--;
		}
	}
	setStringList(lines);

	canceledEditDialog();
}
