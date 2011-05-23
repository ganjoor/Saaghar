﻿/***************************************************************************
 *  This file is part of Saaghar, a Persian poetry software                *
 *                                                                         *
 *  Copyright (C) 2010-2011 by S. Razi Alavizadeh                          *
 *  E-Mail: <s.r.alavizadeh@gmail.com>, WWW: <http://pojh.iBlogger.org>       *
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 3 of the License,         *
 *  (at your option) any later version                                     *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details                            *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program; if not, see http://www.gnu.org/licenses/      *
 *                                                                         *
 ***************************************************************************/
 
#include "SearchItemDelegate.h"
#include "SaagharWidget.h"
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QScrollBar>
#include <QMessageBox>
#include <QAction>
#include <QFile>

//STATIC Variables
QString SaagharWidget::poetsImagesDir = QString();
QLocale  SaagharWidget::persianIranLocal = QLocale();
QFont SaagharWidget::tableFont = qApp->font();
bool SaagharWidget::showBeytNumbers = true;
bool SaagharWidget::centeredView = true;
bool SaagharWidget::backgroundImageState = false;
bool SaagharWidget::newSearchFlag = false;
bool SaagharWidget::newSearchSkipNonAlphabet = false;
QString SaagharWidget::backgroundImagePath = QString();
QColor SaagharWidget::textColor = QColor();
QColor SaagharWidget::matchedTextColor = QColor();
QColor SaagharWidget::backgroundColor = QColor();
QTableWidgetItem *SaagharWidget::lastOveredItem = 0;
int SaagharWidget::maxPoetsPerGroup = 12;
//ganjoor data base browser
QGanjoorDbBrowser *SaagharWidget::ganjoorDataBase = NULL;

//Constants
const Qt::ItemFlags numItemFlags = Qt::NoItemFlags;
const Qt::ItemFlags catsItemFlag = Qt::ItemIsEnabled;
const Qt::ItemFlags poemsItemFlag = Qt::ItemIsEnabled;
const Qt::ItemFlags versesItemFlag = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
const Qt::ItemFlags poemsTitleItemFlag = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

SaagharWidget::SaagharWidget(QWidget *parent, QToolBar *catsToolBar, QTableWidget *tableWidget)
	: QWidget(parent), thisWidget(parent), parentCatsToolBar(catsToolBar), tableViewWidget(tableWidget)
{
	minMesraWidth = 0;
	singleColumnHeightMap.clear();

	loadSettings();

	currentCat = currentPoem = 0;
	currentCaption = tr("Home");

	showHome();
}

SaagharWidget::~SaagharWidget()
{
}

void SaagharWidget::loadSettings()
{
	tableViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	//tableViewWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);
	QPalette p(tableViewWidget->palette());

#ifdef Q_WS_MAC
	if ( SaagharWidget::backgroundImageState && !SaagharWidget::backgroundImagePath.isEmpty() )
	{
		p.setBrush(QPalette::Base, QBrush(QPixmap(SaagharWidget::backgroundImagePath)) );
	}
	else
	{
		p.setColor(QPalette::Base, SaagharWidget::backgroundColor );
	}
	p.setColor(QPalette::Text, SaagharWidget::textColor );
#endif

	if ( SaagharWidget::backgroundImageState && !SaagharWidget::backgroundImagePath.isEmpty() )
	{
		connect(tableViewWidget->verticalScrollBar()		, SIGNAL(valueChanged(int)), tableViewWidget->viewport(), SLOT(update()));
		connect(tableViewWidget->horizontalScrollBar()		, SIGNAL(valueChanged(int)), tableViewWidget->viewport(), SLOT(update()));
	}
	else
	{
		disconnect(tableViewWidget->verticalScrollBar()		, SIGNAL(valueChanged(int)), tableViewWidget->viewport(), SLOT(update()));
		disconnect(tableViewWidget->horizontalScrollBar()	, SIGNAL(valueChanged(int)), tableViewWidget->viewport(), SLOT(update()));
	}
	tableViewWidget->setPalette(p);
	tableViewWidget->setFont(SaagharWidget::tableFont);

	QHeaderView *header = tableViewWidget->verticalHeader();
	header->hide();
	header->setDefaultSectionSize((tableViewWidget->fontMetrics().height()*5)/4);
	tableViewWidget->setVerticalHeader(header);
}

void SaagharWidget::processClickedItem(QString type, int id, bool noError)
{
	if (type == "PoemID" || type == "CatID")
		clearSaagharWidget();
	if ( type == "PoemID")
	{
		GanjoorPoem poem;
		poem.init(0, 0, "", "", false, "");
		if (noError)
			poem = ganjoorDataBase->getPoem(id);
		showPoem(poem);
	}
	else if ( type == "CatID")
	{
		GanjoorCat category;
		category.init(0, 0, "", 0, "");
		if (noError)
			category = ganjoorDataBase->getCategory(id);
		showCategory(category);
	}
}

void SaagharWidget::parentCatClicked()
{
	parentCatButton = qobject_cast<QPushButton *>(sender());
	QString parentCatButtonData = parentCatButton->objectName();
	if (!parentCatButtonData.startsWith("CATEGORY_ID=")) return;
	parentCatButtonData.remove("CATEGORY_ID=");

	GanjoorCat category;
	category.init(0, 0, "", 0, "");
	bool OK = false;
	int CatID = parentCatButtonData.toInt(&OK);

	//qDebug() << "parentCatButtonData=" << parentCatButtonData << "CatID=" << CatID;

	if (OK && ganjoorDataBase)
		category = ganjoorDataBase->getCategory(CatID);

	clearSaagharWidget();
	showCategory(category);
}

void SaagharWidget::showHome()
{
	GanjoorCat homeCat;
	homeCat.init(0, 0, "", 0, "");
	clearSaagharWidget();
	showCategory(homeCat);
}

bool SaagharWidget::nextPoem()
{
	if (ganjoorDataBase->isConnected())
	{
		GanjoorPoem poem = ganjoorDataBase->getNextPoem(currentPoem, currentCat);
		if (!poem.isNull())
		{
			clearSaagharWidget();
			showPoem(poem);
			return true;
		}
	}
	return false;
}

bool SaagharWidget::previousPoem()
{
	if (ganjoorDataBase->isConnected())
	{
		GanjoorPoem poem = ganjoorDataBase->getPreviousPoem(currentPoem, currentCat);
		if (!poem.isNull())
		{
			clearSaagharWidget();
			showPoem(poem);
			return true;
		}
	}
	return false;
}

bool SaagharWidget::initializeCustomizedHome()
{
	if (SaagharWidget::maxPoetsPerGroup == 0) return false;

	QList<GanjoorPoet *> poets = SaagharWidget::ganjoorDataBase->getPoets();
	
	//tableViewWidget->clearContents();
	
	int numOfPoets = poets.size();
	int startIndex = 0;
	int numOfColumn, numOfRow;
	if ( numOfPoets > SaagharWidget::maxPoetsPerGroup)
	{
		if (SaagharWidget::maxPoetsPerGroup == 1)
			startIndex = 0;
		else
			startIndex = 1;

		numOfRow = SaagharWidget::maxPoetsPerGroup+startIndex;//'startIndex=1' is for group title
		if ( (numOfPoets/SaagharWidget::maxPoetsPerGroup)*SaagharWidget::maxPoetsPerGroup == numOfPoets)
			numOfColumn = numOfPoets/SaagharWidget::maxPoetsPerGroup;
		else
			numOfColumn = 1 + numOfPoets/SaagharWidget::maxPoetsPerGroup;
	}
	else
	{
		/*numOfColumn = 1;
		numOfRow = numOfPoets;*/
		return false;
	}
	tableViewWidget->setColumnCount( numOfColumn );
	tableViewWidget->setRowCount( numOfRow );
	
	int poetIndex = 0;
	for (int col=0; col<numOfColumn; ++col)
	{
		QString groupLabel="";
		for (int row=0; row<SaagharWidget::maxPoetsPerGroup; ++row)
		{
			if (startIndex==1)
			{
				if (row == 0)
					groupLabel = poets.at(poetIndex)->_Name;//.at(0);
				if (row == SaagharWidget::maxPoetsPerGroup-1 || poetIndex == numOfPoets-1)
				{
					QString tmp = poets.at(poetIndex)->_Name;

					if (groupLabel != tmp)
					{
						int index = 0;
						while (groupLabel.at(index) == tmp.at(index))
						{
							++index;
							if (index>=groupLabel.size() || index>=tmp.size())
							{
								--index;
								break;
							}
						}
						groupLabel = groupLabel.left(index+1)+"-"+tmp.left(index+1);
					}
					else
						groupLabel = groupLabel.at(0);
					
					
					QTableWidgetItem *groupLabelItem = new QTableWidgetItem( groupLabel );
					groupLabelItem->setFlags(Qt::NoItemFlags);
					groupLabelItem->setTextAlignment(Qt::AlignCenter);
					tableViewWidget->setItem(0, col, groupLabelItem);
				}
			}
			QTableWidgetItem *catItem = new QTableWidgetItem(poets.at(poetIndex)->_Name);
			catItem->setFlags(catsItemFlag);
			catItem->setData(Qt::UserRole, "CatID="+QString::number(poets.at(poetIndex)->_CatID));
			//poets.at(poetIndex)->_ID
			QString poetPhotoFileName = poetsImagesDir+"/"+QString::number(poets.at(poetIndex)->_ID)+".png";;
			if (!QFile::exists(poetPhotoFileName))
				poetPhotoFileName = ":/resources/images/no-photo.png";
			catItem->setIcon(QIcon(poetPhotoFileName));
			
			tableViewWidget->setItem(row+startIndex, col, catItem);
			++poetIndex;
			if (poetIndex>=numOfPoets)
			{
				for (int i=row+1; i<SaagharWidget::maxPoetsPerGroup; ++i)
				{
					QTableWidgetItem *tmpItem = new QTableWidgetItem("");
					tmpItem->setFlags(Qt::NoItemFlags);
					tableViewWidget->setItem(i+startIndex, col, tmpItem);
				}
				break;
			}

			if (col == 0)
				tableViewWidget->setRowHeight(row+startIndex, 105);
		}
		if (poetIndex>=numOfPoets)
				break;
	}
	tableViewWidget->resizeColumnsToContents();
	//tableViewWidget->resizeRowsToContents();
	return true;
}

void SaagharWidget::homeResizeColsRows()
{
	int numOfCols = tableViewWidget->columnCount();
	int numOfRows = tableViewWidget->rowCount();
	int startIndex = 0;
	if (numOfCols > 1 && SaagharWidget::maxPoetsPerGroup != 1)
		startIndex = 1;

	for (int col=0; col<numOfCols; ++col)
	{
		for ( int row=0; row<numOfRows; ++row)
		{
			if(row>=startIndex)
				tableViewWidget->setRowHeight(row, 105);
		}
	}
	tableViewWidget->resizeColumnsToContents();
}

void SaagharWidget::showCategory(GanjoorCat category)
{
	if ( category.isNull() )
	{
		showHome();
		return;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	showParentCategory(category);
	currentPoem = 0;//from showParentCategory

	//new Caption
	currentCaption = (currentCat == 0) ? tr("Home") : ganjoorDataBase->getPoetForCat(currentCat)._Name;//for Tab Title
	emit captionChanged();
	QList<GanjoorCat *> subcats = ganjoorDataBase->getSubCategories(category._ID);

	int subcatsSize = subcats.size();

	bool poetForCathasDescription = false;
	if (category._ParentID == 0)
	{
		poetForCathasDescription = !ganjoorDataBase->getPoetForCat(category._ID)._Description.isEmpty();
	}
	if (subcatsSize == 1 && (category._ParentID != 0 || (category._ParentID == 0 && !poetForCathasDescription)) )
	{
		GanjoorCat firstCat;
		firstCat.init( subcats.at(0)->_ID, subcats.at(0)->_PoetID, subcats.at(0)->_Text, subcats.at(0)->_ParentID,  subcats.at(0)->_Url);
		clearSaagharWidget();
		showCategory(firstCat);
		QApplication::restoreOverrideCursor();
		return;
	}

	QList<GanjoorPoem *> poems = ganjoorDataBase->getPoems(category._ID);

	if (poems.size() == 1 && subcatsSize == 0 && (category._ParentID != 0 || (category._ParentID == 0 && !poetForCathasDescription)))
	{
		GanjoorPoem firstPoem;
		firstPoem.init( poems.at(0)->_ID, poems.at(0)->_CatID, poems.at(0)->_Title, poems.at(0)->_Url,  poems.at(0)->_Faved,  poems.at(0)->_HighlightText);
		clearSaagharWidget();
		showPoem(firstPoem);
		QApplication::restoreOverrideCursor();
		return;
	}
	///Initialize Table//TODO: I need to check! maybe it's not needed
	tableViewWidget->clearContents();
	tableViewWidget->setFrameShape(QFrame::NoFrame);
	tableViewWidget->setFrameStyle(QFrame::NoFrame|QFrame::Plain);
	tableViewWidget->setShowGrid(false);

	QHeaderView *header = tableViewWidget->horizontalHeader();
	header->setStretchLastSection(true);
	header->hide();
	tableViewWidget->setHorizontalHeader(header);
	header = tableViewWidget->verticalHeader();
	header->hide();
	tableViewWidget->setVerticalHeader(header);

	tableViewWidget->setAttribute(Qt::WA_OpaquePaintEvent, true);
	tableViewWidget->setLayoutDirection(Qt::RightToLeft);

	int startRow = 0;
	if (currentCat == 0)
	{
		tableViewWidget->setIconSize(QSize(82, 100));
		bool customHome = true;
		if (customHome)
		{
			tableViewWidget->horizontalHeader()->setStretchLastSection(false);
			if (initializeCustomizedHome())
			{
				QApplication::restoreOverrideCursor();
				return;
			}
			tableViewWidget->horizontalHeader()->setStretchLastSection(true);
		}
		tableViewWidget->setColumnCount(1);
		tableViewWidget->setRowCount(subcatsSize+poems.size());
	}
	else
	{
		tableViewWidget->setColumnCount(1);
		GanjoorPoet gPoet = SaagharWidget::ganjoorDataBase->getPoetForCat(category._ID);
		QString itemText = gPoet._Description;
		if (!itemText.isEmpty() && category._ParentID==0)
		{
			startRow = 1;
			tableViewWidget->setRowCount(1+subcatsSize+poems.size());
			QTableWidgetItem *catItem = new QTableWidgetItem(itemText);
			catItem->setFlags(catsItemFlag);
			catItem->setTextAlignment(Qt::AlignJustify);
			catItem->setData(Qt::UserRole, "CatID="+QString::number(category._ID));
			QString poetPhotoFileName = poetsImagesDir+"/"+QString::number(gPoet._ID)+".png";;
			if (!QFile::exists(poetPhotoFileName))
				poetPhotoFileName = ":/resources/images/no-photo.png";
			catItem->setIcon(QIcon(poetPhotoFileName));
			//set row height
			int textWidth = tableViewWidget->fontMetrics().boundingRect(itemText).width();
			int verticalScrollBarWidth=0;
			if ( tableViewWidget->verticalScrollBar()->isVisible() )
			{
				verticalScrollBarWidth=tableViewWidget->verticalScrollBar()->width();
			}
			int totalWidth = tableViewWidget->columnWidth(0)-verticalScrollBarWidth-82;
			totalWidth = qMax(82+verticalScrollBarWidth, totalWidth);
			//int numOfRow = textWidth/totalWidth ;
			tableViewWidget->setRowHeight(0, qMax(100, SaagharWidget::computeRowHeight(tableViewWidget->fontMetrics(), textWidth, totalWidth)) );
			//tableViewWidget->setRowHeight(0, 2*tableViewWidget->rowHeight(0)+(tableViewWidget->fontMetrics().height()*(numOfRow/*+1*/)));

			tableViewWidget->setItem(0, 0, catItem);
		}
		else
			tableViewWidget->setRowCount(subcatsSize+poems.size());
	}

	for (int i = 0; i < subcatsSize; ++i)
	{
		QTableWidgetItem *catItem = new QTableWidgetItem(subcats.at(i)->_Text);
		catItem->setFlags(catsItemFlag);
		catItem->setData(Qt::UserRole, "CatID="+QString::number(subcats.at(i)->_ID));
		
		if (currentCat == 0)
		{
			GanjoorPoet gPoet = SaagharWidget::ganjoorDataBase->getPoetForCat(subcats.at(i)->_ID);
			QString poetPhotoFileName = poetsImagesDir+"/"+QString::number(gPoet._ID)+".png";
			if (!QFile::exists(poetPhotoFileName))
				poetPhotoFileName = ":/resources/images/no-photo.png";
			catItem->setIcon(QIcon(poetPhotoFileName));
		}
		tableViewWidget->setItem(i+startRow, 0, catItem);

		//freeing resuorce
		delete subcats[i];
		subcats[i]=0;
	}

	for (int i = 0; i < poems.size(); i++)
	{
		QString itemText = poems.at(i)->_Title;
		if (subcatsSize>0)
			itemText.prepend("       ");//7 spaces
		itemText+= " : " + ganjoorDataBase->getFirstMesra(poems.at(i)->_ID);

		QTableWidgetItem *poemItem = new QTableWidgetItem(itemText);
		poemItem->setFlags(poemsItemFlag);
		poemItem->setData(Qt::UserRole, "PoemID="+QString::number(poems.at(i)->_ID));

		//we need delete all
		delete poems[i];
		poems[i]=0;

		tableViewWidget->setItem(subcatsSize+i+startRow, 0, poemItem);
	}

	if (!poems.isEmpty() || !subcats.isEmpty() )
	{
		resizeTable(tableViewWidget);
	}
	QApplication::restoreOverrideCursor();

	emit navPreviousActionState( !ganjoorDataBase->getPreviousPoem(currentPoem, currentCat).isNull() );
	emit navNextActionState( !ganjoorDataBase->getNextPoem(currentPoem, currentCat).isNull() );
}

void SaagharWidget::showParentCategory(GanjoorCat category)
{
	parentCatsToolBar->clear();
	
	//the parents of this category
	QList<GanjoorCat> ancestors = ganjoorDataBase->getParentCategories(category);

	for (int i = 0; i < ancestors.size(); i++)
	{
		parentCatButton = new QPushButton(parentCatsToolBar);
		parentCatButton->setText(ancestors.at(i)._Text);
		parentCatButton->setObjectName("CATEGORY_ID="+QString::number(ancestors.at(i)._ID));//used as button data
		connect(parentCatButton, SIGNAL(clicked(bool)), this, SLOT(parentCatClicked()));
		//style
		int minWidth = parentCatButton->fontMetrics().width(ancestors.at(i)._Text)+6;
		parentCatButton->setStyleSheet(QString("QPushButton{border: 2px solid #8f8f91; border-radius: 6px; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde); min-width: %1px; min-height: %2px; text margin-left:1px; margin-right:1px } QPushButton:pressed { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #dadbde, stop: 1 #f6f7fa); } QPushButton:flat { border: none; } QPushButton:default { border-color: red; }").arg(minWidth).arg(parentCatButton->fontMetrics().height()+2));
		
		parentCatsToolBar->addWidget(parentCatButton);
	}

	if (!category._Text.isEmpty())
	{
		parentCatButton = new QPushButton(parentCatsToolBar);
		parentCatButton->setText(category._Text);
		parentCatButton->setObjectName("CATEGORY_ID="+QString::number(category._ID));//used as button data
		connect(parentCatButton, SIGNAL(clicked(bool)), this, SLOT(parentCatClicked()));
		int minWidth = parentCatButton->fontMetrics().width(category._Text)+6;
		parentCatButton->setStyleSheet(QString("QPushButton{border: 2px solid #8f8f91; border-radius: 6px; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde); min-width: %1px; min-height: %2px; text margin-left:1px; margin-right:1px } QPushButton:pressed { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #dadbde, stop: 1 #f6f7fa); } QPushButton:flat { border: none; } QPushButton:default { border-color: red; }").arg(minWidth).arg(parentCatButton->fontMetrics().height()+2));
		parentCatsToolBar->addWidget(parentCatButton);
	}

	currentCat = !category.isNull() ? category._ID : 0;
}

int SaagharWidget::showPoem(GanjoorPoem poem)
{
	if ( poem.isNull() )
	{
	return 0;
	}
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	showParentCategory(ganjoorDataBase->getCategory(poem._CatID));

	QList<GanjoorVerse *> verses = ganjoorDataBase->getVerses(poem._ID);

	QFontMetrics fontMetric(tableViewWidget->fontMetrics());

	int WholeBeytNum = 0;
	int BeytNum = 0;
	int BandNum = 0;
	int BandBeytNums = 0;
	bool MustHave2ndBandBeyt = false;
	int MissedMesras = 0;

	tableViewWidget->setRowCount(2);
	tableViewWidget->setColumnCount(4);

	//new Caption
	currentCaption = (currentCat == 0) ? tr("Home") : ganjoorDataBase->getPoetForCat(currentCat)._Name;//for Tab Title
	currentCaption += ":"+poem._Title.left(7);
	emit captionChanged();

	//disabling item (0,0)
	QTableWidgetItem *flagItem = new QTableWidgetItem("");
	flagItem->setFlags(Qt::NoItemFlags);
	tableViewWidget->setItem(0, 0, flagItem);

	//Title of Poem
	QTableWidgetItem *poemTitle = new QTableWidgetItem(poem._Title);
	poemTitle->setFlags(poemsTitleItemFlag);
	poemTitle->setData(Qt::UserRole, "PoemID="+QString::number(poem._ID));
	if (centeredView)
		poemTitle->setTextAlignment(Qt::AlignCenter);
	else
		poemTitle->setTextAlignment(Qt::AlignLeft|Qt::AlignCenter);

	int textWidth = fontMetric.boundingRect(poem._Title).width();
	int totalWidth = tableViewWidget->columnWidth(1)+tableViewWidget->columnWidth(2)+tableViewWidget->columnWidth(3);
	//int numOfRow = textWidth/totalWidth ;

	tableViewWidget->setItem(0, 1, poemTitle);
	tableViewWidget->setSpan(0, 1, 1, 3 );
	tableViewWidget->setRowHeight(0, SaagharWidget::computeRowHeight(tableViewWidget->fontMetrics(), textWidth, totalWidth) );
	//tableViewWidget->rowHeight(0)+(fontMetric.height()*(numOfRow/*+1*/)));

	int row = 1;

	tableViewWidget->setLayoutDirection(Qt::RightToLeft);

	int firstEmptyThirdColumn = 1;
	bool flagEmptyThirdColumn = false;
	//very Big For loop
	minMesraWidth = 0;
	singleColumnHeightMap.clear();

#ifndef Q_WS_MAC //Qt Bug when inserting TATWEEl character
	const bool justified = true;//temp
	int maxWidth = -1;
	if (justified)
	{
		for (int i = 0; i < verses.size(); i++)
		{
			QString verseText = verses.at(i)->_Text;
			
			if (verses.at(i)->_Position == Single || verses.at(i)->_Position == Paragraph) continue;
			int temp = fontMetric.width(verseText);
			if (temp>maxWidth)
				maxWidth = temp;
		}
	}
#endif

	for (int i = 0; i < verses.size(); i++)
	{
		QString currentVerseText = verses.at(i)->_Text;

#ifndef Q_WS_MAC //Qt Bug when inserting TATWEEl character
		if (justified)
		{
			currentVerseText = QGanjoorDbBrowser::justifiedText(currentVerseText, fontMetric, maxWidth);
		}
#endif

		if (currentVerseText.isEmpty())
		{
			if (verses.at(i)->_Position == Paragraph
				|| verses.at(i)->_Position == CenteredVerse1
				|| verses.at(i)->_Position == CenteredVerse2
				|| verses.at(i)->_Position == Single )
			{
				if (i == verses.size()-1)
					tableViewWidget->removeRow(row);
				continue;
			}

			if (verses.at(i)->_Position == Left)
			{
				bool empty = true;
				for (int k=0; k < tableViewWidget->columnCount(); ++k)
				{
					QTableWidgetItem *temp = tableViewWidget->item(row, k);
					if (temp && temp->data(Qt::DisplayRole).isValid() && !temp->data(Qt::DisplayRole).toString().isEmpty() )
						empty = false;
				}

				if (empty)
				{
					if (i == verses.size()-1)
						tableViewWidget->removeRow(row);
					continue;
				}
			}
		}

		QString extendString = QString::fromLocal8Bit("پپ");//mesra width computation

		QTableWidgetItem *mesraItem = new QTableWidgetItem(currentVerseText);
		mesraItem->setFlags(versesItemFlag);
		//set data for mesraItem
		QString verseData = QString::number(verses.at(i)->_PoemID)+"|"+QString::number(verses.at(i)->_Order)+"|"+QString::number((int)verses.at(i)->_Position);
		mesraItem->setData(Qt::UserRole, "VerseData=|"+verseData);

		bool spacerColumnIsPresent = false;

		if (centeredView)
		{
			switch (verses.at(i)->_Position)
			{
				case Right :
					spacerColumnIsPresent = true;
					if (!flagEmptyThirdColumn)
					{
						firstEmptyThirdColumn = row;
						flagEmptyThirdColumn = true;
					}
					if (fontMetric.width(currentVerseText+extendString) > minMesraWidth)
						minMesraWidth = fontMetric.width(currentVerseText+extendString);
					mesraItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
					tableViewWidget->setItem(row, 1, mesraItem);
					if (MustHave2ndBandBeyt)
					{
						MissedMesras++;
						MustHave2ndBandBeyt = false;
					}
					break;

				case Left :
					spacerColumnIsPresent = true;
					if (!flagEmptyThirdColumn)
					{
						firstEmptyThirdColumn = row;
						flagEmptyThirdColumn = true;
					}
					if (fontMetric.width(currentVerseText+extendString) > minMesraWidth)
						minMesraWidth = fontMetric.width(currentVerseText+extendString);
					mesraItem->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
					tableViewWidget->setItem(row, 3, mesraItem);
					break;

				case CenteredVerse1 :
					mesraItem->setTextAlignment(Qt::AlignCenter);
					tableViewWidget->setItem(row, 1, mesraItem);
					tableViewWidget->setSpan(row, 1, 1, 3 );
					BandBeytNums++;
					MustHave2ndBandBeyt = true;
					break;

				case CenteredVerse2 :
					mesraItem->setTextAlignment(Qt::AlignCenter);
					tableViewWidget->setItem(row, 1, mesraItem);
					tableViewWidget->setSpan(row, 1, 1, 3 );
					MustHave2ndBandBeyt = false;
					break;

				case Single :
					textWidth = fontMetric.boundingRect(mesraItem->data(Qt::DisplayRole).toString()).width();
					totalWidth = tableViewWidget->columnWidth(1)+tableViewWidget->columnWidth(2)+tableViewWidget->columnWidth(3);
					//numOfRow = textWidth/totalWidth ;
					if (!currentVerseText.isEmpty())
					{
						tableViewWidget->setItem(row, 1, mesraItem);
						tableViewWidget->setSpan(row, 1, 1, 3 );
					}
					
					tableViewWidget->setRowHeight(row, SaagharWidget::computeRowHeight(fontMetric, textWidth/*mesraItem->data(Qt::DisplayRole).toString()*/, totalWidth/*, tableViewWidget->rowHeight(row)*/));
						//					tableViewWidget->setRowHeight(row, tableViewWidget->rowHeight(row)+(fontMetric.height()*(numOfRow/*+1*/)));
					singleColumnHeightMap.insert(row, textWidth/*tableViewWidget->rowHeight(row)*/);
					MissedMesras--;
					break;

				case Paragraph :
					textWidth = fontMetric.boundingRect(mesraItem->data(Qt::DisplayRole).toString()).width();
					totalWidth = tableViewWidget->columnWidth(1)+tableViewWidget->columnWidth(2)+tableViewWidget->columnWidth(3);
					//numOfRow = textWidth/totalWidth;
					{
						/*QTextEdit *paraEdit = new QTextEdit(this);
						paraEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
						paraEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
						paraEdit->setFrameShape(QFrame::NoFrame);
						paraEdit->setPlainText(mesraItem->text());
						tableViewWidget->setCellWidget(row, 1, paraEdit);*/
						QTableWidgetItem *tmp = new QTableWidgetItem("");
						tmp->setFlags(versesItemFlag);
						tableViewWidget->setItem(row, 3, tmp);
						mesraItem->setTextAlignment(Qt::AlignVCenter|Qt::AlignLeft);
						tableViewWidget->setItem(row, 1, mesraItem);
						tableViewWidget->setSpan(row, 1, 1, 3 );
					}
					tableViewWidget->setRowHeight(row, SaagharWidget::computeRowHeight(fontMetric, textWidth/*mesraItem->data(Qt::DisplayRole).toString()*/, totalWidth/*, tableViewWidget->rowHeight(row)*/));
					//tableViewWidget->setRowHeight(row, tableViewWidget->rowHeight(row)+(fontMetric.height()*(numOfRow/*+1*/)));
					singleColumnHeightMap.insert(row, textWidth/*tableViewWidget->rowHeight(row)*/);
					MissedMesras++;
					break;

				default:
					break;
			}

			if (spacerColumnIsPresent)
			{
				QTableWidgetItem *tmp = new QTableWidgetItem("");
				tmp->setFlags(Qt::NoItemFlags);
				tableViewWidget->setItem(row, 2, tmp);			
			}
		}// end of centeredView

		QString simplifiedText = currentVerseText;
		simplifiedText.remove(" ");
		simplifiedText.remove("\t");
		simplifiedText.remove("\n");

		if ( !simplifiedText.isEmpty() && (( verses.at(i)->_Position == Single && !currentVerseText.isEmpty() ) ||
			verses.at(i)->_Position == Right ||
			verses.at(i)->_Position == CenteredVerse1)
			)
		{
			WholeBeytNum++;
			bool isBand = (verses.at(i)->_Position == CenteredVerse1);
			if (isBand)
			{
				BeytNum = 0;
				BandNum++;
			}
			else
				BeytNum++;

			if ( showBeytNumbers && !currentVerseText.isEmpty() )
			{//empty verse strings have been seen sometimes, it seems that we have some errors in our database
				int itemNumber = isBand ? BandNum : BeytNum;
				//QString verseData = QString::number(verses.at(i)->_PoemID)+"."+QString::number(verses.at(i)->_Order)+"."+QString::number((int)verses.at(i)->_Position);

				QString localizedNumber = SaagharWidget::persianIranLocal.toString(itemNumber);
				QTableWidgetItem *numItem = new QTableWidgetItem(localizedNumber);
				if (isBand)
				{
					QFont fnt = numItem->font();
					fnt.setBold(true);
					fnt.setItalic(true);
					numItem->setFont(fnt);
					numItem->setForeground(QBrush(QColor(Qt::yellow).darker(150)));
				}
				numItem->setFlags(numItemFlags);
				tableViewWidget->setItem(row, 0, numItem);
			}
		}

		if (verses.at(i)->_Position == Paragraph ||
			verses.at(i)->_Position == Left ||
			verses.at(i)->_Position == CenteredVerse1 ||
			verses.at(i)->_Position == CenteredVerse2 ||
			verses.at(i)->_Position == Single )
		{	
			QTableWidgetItem *numItem = tableViewWidget->item(row, 0 );
			if (!numItem)
			{
				numItem = new QTableWidgetItem("");
				numItem->setFlags(Qt::NoItemFlags);
				tableViewWidget->setItem(row, 0, numItem);
			}

			++row;
			if (i != verses.size()-1)
				tableViewWidget->insertRow(row);
		}

		//delete Verses
		delete verses[i];
		verses[i]=0;
	}// end of big for


	//a trick for removing last empty row
	QString rowStr = "";
	for (int col = 0; col < tableViewWidget->columnCount(); ++col)
	{
		QTableWidgetItem *tmpItem = tableViewWidget->item(tableViewWidget->rowCount()-1, col);
		if (tmpItem)
		{
			rowStr+=tmpItem->text();
		}
	}
	rowStr.remove(" ");
	rowStr.remove("\t");
	rowStr.remove("\n");
	if (rowStr.isEmpty())
	{
		tableViewWidget->setRowCount(tableViewWidget->rowCount()-1);
	}

	resizeTable(tableViewWidget);

	QApplication::restoreOverrideCursor();

	currentPoem = poem._ID;

	emit navPreviousActionState( !ganjoorDataBase->getPreviousPoem(currentPoem, currentCat).isNull() );
	emit navNextActionState( !ganjoorDataBase->getNextPoem(currentPoem, currentCat).isNull() );

	return 0;
}

void SaagharWidget::clearSaagharWidget()
{
	lastOveredItem = 0;
	tableViewWidget->setRowCount(0);
	tableViewWidget->setColumnCount(0);
}

QString SaagharWidget::currentPageGanjoorUrl ()
{
	if(currentCat == 0)
		return "http://ganjoor.net";
	if (currentPoem == 0)
		return ganjoorDataBase->getCategory(currentCat)._Url;
	return ganjoorDataBase->getPoem(currentPoem)._Url;
	/*
	 * using following code you can delete url field in poem table,
	 *
	 * return "http://ganjoor.net/?p=" + currentPoem.ToString();
	 *
	 * it causes ganjoor.net to perform a redirect to SEO frinedly url,
	 * however size of database is reduced only by 2 mb (for 69 mb one) in this way
	 * so I thought it might not condisered harmful to keep current structure.
	 */
}

void SaagharWidget::resizeTable(QTableWidget *table)
{
	if (table && table->columnCount()>0)
	{
		if (currentCat == 0  && currentPoem == 0 ) //it's Home.
		{
			//table->resizeRowsToContents();
			homeResizeColsRows();
			return;
		}

		QString vV="False";
		int verticalScrollBarWidth=0;
		if ( table->verticalScrollBar()->isVisible() )
		{
			vV="true";
			verticalScrollBarWidth=table->verticalScrollBar()->width();
		}
		int baseWidthSize=thisWidget->width()-(2*table->x()+verticalScrollBarWidth);
		//int tW=0;
		//for (int i=0; i<table->columnCount(); ++i)
		//{
		//	tW+=table->columnWidth(i);
		//}
		//qDebug() << QString("x=*%1*--w=*%2*--vX=*%3*--v-W=*%4*--Scroll=*%5*--verticalScrollBarWidth=*%6*--baseWidthSize=*%7*\ntW=*%8*--tableW=*%9*").arg(table->x()).arg(thisWidget->width()/* width()-(2*table->viewport()->x())*/).arg(table->viewport()->x()).arg(table->viewport()->width()).arg(vV).arg(verticalScrollBarWidth).arg(baseWidthSize).arg(tW).arg(-1 );
		
		//resize description's row
		if (currentCat != 0  && currentPoem == 0 && table->columnCount() == 1)
		{//using column count here is a tricky way
			GanjoorCat gCat = SaagharWidget::ganjoorDataBase->getCategory(currentCat);
			if (gCat._ParentID == 0)
			{
				GanjoorPoet gPoet = SaagharWidget::ganjoorDataBase->getPoetForCat(gCat._ID);
				QString itemText = gPoet._Description;
				if (!itemText.isEmpty())
				{
					int textWidth = table->fontMetrics().boundingRect(itemText).width();
					int verticalScrollBarWidth=0;
					if ( table->verticalScrollBar()->isVisible() )
					{
						verticalScrollBarWidth=table->verticalScrollBar()->width();
					}
					int totalWidth = table->columnWidth(0)-verticalScrollBarWidth-82;
					totalWidth = qMax(82+verticalScrollBarWidth, totalWidth);
					table->setRowHeight(0, qMax(100, SaagharWidget::computeRowHeight(table->fontMetrics(), textWidth, totalWidth)) );
				}
			}
		}

		//resize rows that contains 'Paragraph' and 'Single'
		int totalWidth = 0;
		if (table->columnCount() == 4)
		{
			totalWidth = tableViewWidget->columnWidth(1)+tableViewWidget->columnWidth(2)+tableViewWidget->columnWidth(3);
			QMap<int, int>::const_iterator i = singleColumnHeightMap.constBegin();
			while (i != singleColumnHeightMap.constEnd())
			{
				table->setRowHeight(i.key(), SaagharWidget::computeRowHeight(table->fontMetrics(), i.value(), totalWidth /*, table->rowHeight(i.key())*/  ));
				//table->setRowHeight(i.key(), i.value());
				++i;
			}
		}

		switch (table->columnCount())
		{
			case 1:
				table->setColumnWidth(0, qMax(minMesraWidth, baseWidthSize) );//single mesra
				break;
			case 2:
				table->setColumnWidth(0, (baseWidthSize*5)/100);
				table->setColumnWidth(1, (baseWidthSize*94)/100);
				break;
			case 3:
				table->setColumnWidth(0, (baseWidthSize*6)/100);
				table->setColumnWidth(1, (baseWidthSize*47)/100);
				table->setColumnWidth(2, (baseWidthSize*47)/100);
				break;
			case 4:
				table->setColumnWidth(0, table->fontMetrics().width(QString::number(table->rowCount()*10)) );//numbers
				table->setColumnWidth(2, table->fontMetrics().height()*2 /*5/2*/ );//spacing between mesras
				baseWidthSize = baseWidthSize - ( table->columnWidth(0)+table->columnWidth(2) );
				table->setColumnWidth(1, qMax(minMesraWidth, baseWidthSize/2/* -table->columnWidth(0) */) );//mesra width
				table->setColumnWidth(3, qMax(minMesraWidth, baseWidthSize/2) );//mesra width
				break;
			default:
				break;
		}
	}
}

void SaagharWidget::scrollToFirstItemContains(const QString &phrase)
{
	for (int row = 0; row < tableViewWidget->rowCount(); ++row)
	{
		for (int col = 0; col < tableViewWidget->columnCount(); ++col)
		{
			QTableWidgetItem *tmp = tableViewWidget->item(row, col);
			if (newSearchFlag)
			{
				if (tmp)
				{
					QString text = QGanjoorDbBrowser::cleanString(tmp->text(), newSearchSkipNonAlphabet);
					if (text.contains(phrase))
					{
						tableViewWidget->setCurrentItem(tmp, QItemSelectionModel::NoUpdate);
						tableViewWidget->scrollToItem(tmp, QAbstractItemView::PositionAtCenter);
						row = tableViewWidget->rowCount()+1;
						break;
					}
				}
			}
			else
			{
				if (tmp && tmp->text().contains(phrase))
				{
					tableViewWidget->setCurrentItem(tmp, QItemSelectionModel::NoUpdate);
					tableViewWidget->scrollToItem(tmp, QAbstractItemView::PositionAtCenter);
					row = tableViewWidget->rowCount()+1;
					break;
				}
			}
		}
	}
}

int SaagharWidget::computeRowHeight(const QFontMetrics &fontMetric, int textWidth, int width, int height)
{
	if (width <=0 || textWidth<=0) return 4*(fontMetric.height()/3);
	if (height == 0)
		height = (4*fontMetric.height())/3;
	int numOfRow = textWidth/width ;
	return height+((fontMetric.height()*numOfRow));
}
