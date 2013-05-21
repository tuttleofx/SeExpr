/* 
* (c) Disney Enterprises, Inc.  All rights reserved.
*
* This file is licensed under the terms of the Microsoft Public License (MS-PL)
* as defined at: http://opensource.org/licenses/MS-PL.
*
* A complete copy of this license is included in this distribution as the file
* LICENSE.
*/

// NOTE: This is based on Dan's paint3d FileDialog

#include "QdSeFileDialog.h"


#include <QtGui/QToolButton>
#include <QtGui/QPalette>
#include <QtGui/QMenu>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <iostream>

using std::max;
using std::min;

static const char* folder_fav[]={
    "17 16 4 1",
    "# c #000000",
    ". c None",
    "a c #ffff98",
    "b c #cc0000",
    ".................",
    ".................",
    "...#####.........",
    "..#aaaaa#........",
    ".###############.",
    ".#aaaaaaaaaaaaa#.",
    ".#aaaa##a##aaaa#.",
    ".#aaa#bb#bb#aaa#.",
    ".#aaa#bbbbb#aaa#.",
    ".#aaa#bbbbb#aaa#.",
    ".#aaaa#bbb#aaaa#.",
    ".#aaaaa#b#aaaaa#.",
    ".#aaaaaa#aaaaaa#.",
    ".#aaaaaaaaaaaaa#.",
    ".###############.",
    "................."};


void QdSePreviewWidget::makePreview(const QString& path)
{
    QFileInfo fi( path );
    
    if ( fi.isDir() )
    {
        QString s = fi.absoluteFilePath()+"/preview.tif";
        if (!QFile::exists(s)) s = fi.absoluteFilePath()+"/preview.png";
        if (!QFile::exists(s)) _pm->setPixmap( QPixmap()); //nothing to preview
	
        QPixmap pix( s );
        if ( !pix.isNull() ) _pm->setPixmap( pix );
        else _pm->setPixmap( QPixmap() );
    }
    else if (fi.exists())
    {
        QImage img(fi.absoluteFilePath());
        if(!img.isNull())
            _pm->setPixmap(QPixmap::fromImage(img.scaled(128,128,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
        else 
            _pm->setPixmap( QPixmap() );
    }
    else _pm->setPixmap( QPixmap() );
    _pm->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}


QdSePreviewWidget::QdSePreviewWidget( QWidget *parent )
    : QWidget( parent )
{
    _pm = new QLabel( this );
    _pm->setFrameStyle( QFrame::StyledPanel );
    _pm->setBackgroundRole( QPalette::Base );
    _pm->setAutoFillBackground(true);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing( 0 );
    layout->setMargin( 0 );
    layout->addWidget(_pm);
    setLayout(layout);
}

QdSeFileDialog::QdSeFileDialog(QWidget* parent) : QFileDialog(parent)
{
    //QStringList pathlist(QString(globals.startpath.c_str()));
    //addLookInEntries(pathlist);
    
    _nameEdit = 0;
    _okButton = 0;
    
    // disconnect broken return press handling (mishandles new directory names)
    QList<QLineEdit*> lineedits = findChildren<QLineEdit*>(QRegExp());
    if (lineedits.size()) _nameEdit = (QLineEdit*)lineedits.at(0);
    if (_nameEdit) {
	_nameEdit->disconnect(SIGNAL(returnPressed()));
	connect(_nameEdit, SIGNAL(returnPressed()), SLOT(editReturnPress()));
    }
    
    // connect custom ok clicked handler
    QList<QPushButton*> myWidgets = findChildren<QPushButton*>(QRegExp());
    for (int w = 0; w < myWidgets.size(); w++)
    {
	QPushButton* item = (QPushButton*)myWidgets.at(w);
	if (item->text().contains("Open")) _okButton = item;
    }
    if (_okButton) connect(_okButton, SIGNAL(clicked()), SLOT(handleOk()));
    
    connect(this, SIGNAL(currentChanged(const QString&)),
            this, SLOT(selChanged(const QString&)));

    // don't create missing directories by default
    _createDir = 0;
    _pw = 0;
    _favDir = "";
    _combo = 0;
    _combolabel = 0;
    _cb = 0;
    _temppath = "";
    
    setMinimumWidth(680);
    resize(840,440);
}

void QdSeFileDialog::handleOk()
{
    if (fileMode() != QFileDialog::DirectoryOnly) return;
    QString entry = _nameEdit->text();
    if (entry == "") return;

    // create directory if needed
    if (_createDir) {
	QDir d(directory());
	if (!d.exists(entry)) {
	    if (d.mkdir(entry)) {
		_temppath = directory().absolutePath();
		setDirectory(_temppath+"/"+entry);
		_nameEdit->setText("");
 		if (_okButton)
 		    _okButton->animateClick(); // retry click to accept entry
		    
		QTimer::singleShot( 200, this, SLOT(resetDir()) );
	    }
	}
    }
}

void QdSeFileDialog::editReturnPress()
{
    if (!_nameEdit) return;
    
    QString str = _nameEdit->text();
    if (str.contains('/'))
    {
	QDir d;
	if (d.cd(str))
	{
	    setDirectory(str);
	    _nameEdit->setText("");
	}
        else
	{
	    int slashcount = str.count('/');
	    
	    QString foundDir = "";
	    for (int i=0; i<slashcount; i++)
	    {
		QString section = str.section('/', 0, i);
		if (d.cd(section)) foundDir = section;
	    }
	    if (foundDir.length())
	    {
	        setDirectory(foundDir);
		QString remainder = str.right(str.length()-(foundDir.length()+1));
		_nameEdit->setText(remainder);
	    }
	    
	    if (d.cd(str)) setDirectory(str);
	    
	}
    }
    else if (fileMode()==QFileDialog::DirectoryOnly) handleOk();
    else accept();
}

void QdSeFileDialog::addFavoritesButton(QString dirname, QString linkname,
    QString linkdir)
{
    QGridLayout *layout = findChild<QGridLayout *>("gridLayout");
    if (!layout) return;

    QDir d;
    
    std::string favlocation = getenv("HOME");
    favlocation += "/paint3d/favorites/";
    
    QString dirpath = QString::fromStdString(favlocation);
    if (!d.cd(dirpath)) d.mkpath(dirpath);
    dirpath += dirname;
    if (!d.cd(dirpath)) d.mkpath(dirpath);
    
    if (!(linkdir.isEmpty() || linkname.isEmpty()))
    {
	if (!QFile::exists(dirpath+linkname))
            QFile::link(linkdir,dirpath+linkname);
    }
    
    _favDir = dirpath;
    
    static QPixmap folderFav( folder_fav );
    QToolButton* fav = new QToolButton(this);
    fav->setFixedSize(18,18);
    fav->setIcon(folderFav);
    fav->setToolTip("Favorites");
    
    layout->addWidget(fav,0,3);
    
    connect(fav, SIGNAL(clicked()), SLOT(gotoFavorites()));
}

void QdSeFileDialog::gotoFavorites()
{
    if (_favDir!="") setDirectory( _favDir );
}

void QdSeFileDialog::addLookInEntries(QStringList paths)
{
    if ( paths.isEmpty ()) return;
    
    QStringList h = history();
    for (QStringList::Iterator it=paths.begin(); it!=paths.end(); ++it)
    {
	if (!h.contains(*it)) h.push_back(*it);
    }
    setHistory(h);
}

void QdSeFileDialog::saveLookInEntries()
{
    _lookInList = history();
}

void QdSeFileDialog::restoreLookInEntries()
{
    setHistory(_lookInList);
}

static QStringList makeFiltersList( const QString &filter )
{
    if ( filter.isEmpty() )
	return QStringList();

    int i = filter.indexOf( ";;", 0 );
    QString sep( ";;" );
    if ( i == -1 ) {
	if ( filter.indexOf( "\n", 0 ) != -1 ) {
	    sep = "\n";
	    i = filter.indexOf( sep, 0 );
	}
    }

    return filter.split( sep );
}

QString QdSeFileDialog::getOpenFileName( const QString& caption,
    const QString& startWith, const QString& filter )
{
    if ( !filter.isEmpty() )
    {
	QStringList filters = makeFiltersList( filter );
	setNameFilters( filters );
    }

    if ( !startWith.isEmpty() ) setDirectory( startWith );
    if ( !caption.isNull() ) setWindowTitle( caption );
    setFileMode( QFileDialog::ExistingFile );
    setAcceptMode( QFileDialog::AcceptOpen );
    selectFile("");
    
    QString result;
    if ( exec()==QDialog::Accepted )
    {
	result = selectedFiles().first();
	_workingDirectory = directory().absolutePath();
    }
    resetPreview();

    return result;
}

QStringList QdSeFileDialog::getOpenFileNames( const QString& caption,
    const QString& startWith, const QString& filter )
{
    if ( !filter.isEmpty() )
    {
	QStringList filters = makeFiltersList( filter );
	setNameFilters( filters );
    }

    if ( !startWith.isEmpty() ) setDirectory( startWith );
    if ( !caption.isNull() ) setWindowTitle( caption );
    setFileMode( QFileDialog::ExistingFiles );
    setAcceptMode( QFileDialog::AcceptOpen );
    selectFile("");
    
    QString result;
    QStringList lst;
    if ( exec()==QDialog::Accepted )
    {
	lst = selectedFiles();
	_workingDirectory = directory().absolutePath();
    }
    resetPreview();

    return lst;
}

QString QdSeFileDialog::getExistingDirectory( const QString& caption,
    const QString& startWith, const QString& filter )
{
    if ( !filter.isEmpty() )
    {
	QStringList filters = makeFiltersList( filter );
	setNameFilters( filters );
    }

    if ( !startWith.isEmpty() ) setDirectory( startWith );
    if ( !caption.isNull() ) setWindowTitle( caption );
    setFileMode( QFileDialog::DirectoryOnly );
    selectFile("");
    
    QString result;
    if ( exec()==QDialog::Accepted )
    {
	result = selectedFiles().first();
	_workingDirectory = directory().absolutePath();
    }
    resetPreview();

    return result;
}

QString QdSeFileDialog::getExistingOrNewDirectory( const QString& caption,
    const QString& startWith, const QString& filter )
{
    _createDir = 1;
    QString result = getExistingDirectory(caption, startWith, filter);
    _createDir = 0;
    resetPreview();
    return result;
}

QString QdSeFileDialog::getSaveFileName( const QString& caption,
    const QString& startWith, const QString& filter )
{
    if ( !filter.isEmpty() )
    {
	QStringList filters = makeFiltersList( filter );
	setNameFilters( filters );
    }
    
    if ( !startWith.isEmpty() ) setDirectory( startWith );
    if ( !caption.isNull() ) setWindowTitle( caption );
    setFileMode( QFileDialog::AnyFile );
    setAcceptMode( QFileDialog::AcceptSave );
    selectFile("");
    
    QString result;
    if ( exec()==QDialog::Accepted )
    {
	result = selectedFiles().first();
	_workingDirectory = directory().absolutePath();
    }
    resetPreview();

    return result;
}


void QdSeFileDialog::setPreview()
{
    QGridLayout *layout = findChild<QGridLayout *>("gridLayout");
    if (!layout) return;
    
    _pw = new QdSePreviewWidget( this );
    _pw->setFixedWidth(160);
    _pw->setMinimumHeight(160);
    layout->addWidget(_pw,1,3);
}

void QdSeFileDialog::resetPreview()
{
    if (_pw) _pw->reset();
}

void QdSeFileDialog::addCheckBox(QString s)
{
    QGridLayout *layout = findChild<QGridLayout *>("gridLayout");
    if (!layout) return;
    
    _cb = new QCheckBox( s, this );
    _cb->setChecked(false);
    
    layout->addWidget(_cb, 4, _combo ? 2 : 0);
}

bool QdSeFileDialog::checkBoxStatus()
{
    if (!_cb) return false;
    return _cb->isChecked();
}

void QdSeFileDialog::showCheckBox()
{
    if (_cb) _cb->show();
}

void QdSeFileDialog::hideCheckBox()
{
    if (_cb) _cb->hide();
}

void QdSeFileDialog::addComboBox(QString s, QStringList sl)
{
    QGridLayout *layout = findChild<QGridLayout *>("gridLayout");
    if (!layout) return;
    
    _combolabel = new QLabel( s, this );
    _combolabel->setFixedWidth(58);
    _combo = new QComboBox( this );
    _combo->setEditable(true);
    _combo->setFixedWidth(160);
    for ( QStringList::Iterator it = sl.begin(); it != sl.end(); ++it )
        _combo->addItem(*it);

    int rownum = layout->rowCount();
    layout->addWidget(_combo,rownum,1);
    layout->addWidget(_combolabel,rownum,0);
}

void QdSeFileDialog::showComboBox()
{
    if (_combo) _combo->show();
    if (_combolabel) _combolabel->show();
}

void QdSeFileDialog::hideComboBox()
{
    if (_combo) _combo->hide();
    if (_combolabel) _combolabel->hide();
}

void QdSeFileDialog::selChanged(const QString& path)
{
    if (_pw) _pw->makePreview(path);
}

void QdSeFileDialog::setButtonName(const QString& str)
{
    if (_okButton) _okButton->setText( str );
}

void QdSeFileDialog::addSidebarShortcut(const QString& s)
{
    QList<QUrl> urls = sidebarUrls();
    QUrl url = QUrl::fromLocalFile(s);
    if (url.isValid() && QFile::exists(s))
    {
        urls.push_back(url);
        setSidebarUrls(urls);
    }
}