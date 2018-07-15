#include "PreviewHTMLWindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QReadWriteLock>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QMenu>
#include <QAction>
#include "HTMLViewQt.h"

using future_core::HTMLViewQt;

PreviewHTMLWindow::PreviewHTMLWindow(QWidget * parent, const std::string htmlPath, const QSize& standardSize)
	:QDockWidget(parent),
    m_htmlPath(htmlPath),
	m_standardSize(standardSize),
    m_viewCore(0)
{
	m_innerHtmlPath = tempFilePath(htmlPath);
	QFile f(htmlPath.c_str());
	if (f.exists()) {
		f.copy(m_innerHtmlPath.c_str());
	}
	// Must initial engine at last!
	// in case the width and height value get wrong
    initialCoreView();
    if ( !LayoutEngine::GetEngine()->isEngineReady() ) {
        LayoutEngine::GetEngine()->initLayoutEngine("", [this]() {
            engineInitFinish();
        });
    } else {
        engineInitFinish();
    }
#if __APPLE__
    float ratio = 1;
#else
    float ratio = 0.85;
#endif
    const float margin = 60.f;
    LayoutEngine::GetEngine()->SetViewTopMargin(margin / ratio);
    LayoutEngine::GetEngine()->SetViewBottomMargin(margin / ratio);
	std::string bgPrefix("background-color:");
	std::string bgColorStyle = bgPrefix + m_supportBGColor[0];
	setStyleSheet(QString(bgColorStyle.c_str()));
}

PreviewHTMLWindow::~PreviewHTMLWindow()
{
	cleanResource();
}

void PreviewHTMLWindow::engineInitFinish() {
    LayoutEngine::GetEngine()->openHtml(m_viewCore, m_innerHtmlPath, "html_id_key", [this](HTMLReader* reader, int ecode) {
        engineOpenHTMLFinish(reader, ecode);
    });
}
/*
 * engine html delegate function 
 */

// html open success 
void PreviewHTMLWindow::engineOpenHTMLFinish(HTMLReader *html, int error) {
	if ( error || !html ) {
        return;
	}
    if (m_htmlReader) {
        cleanResource();
        initialCoreView();
    }
    m_htmlReader = html;
    m_viewCore->setHTMLReader(html);
    m_viewCore->SetPaintSize(m_standardSize.width(), m_standardSize.height());
    LayoutEngine::GetEngine()->gotoPageByIndex(m_htmlReader, m_currentPageIndex);
    m_viewCore->updateView();
}

/*
 * system function
 */

// paint event
void PreviewHTMLWindow::paintEvent(QPaintEvent *)
{
    if ( !m_viewCore ) {
        return;
    }
    m_viewCore->onDraw();
}

// resize
void PreviewHTMLWindow::resizeEvent(QResizeEvent *event) 
{
	if (event->size() != event->oldSize()) {
		float ratio = m_standardSize.width() / float(m_standardSize.height());
		resize(event->size().width(), int(event->size().width() / ratio));
	}
}
// mouse click event
void PreviewHTMLWindow::mousePressEvent(QMouseEvent *event)
{
    if (!m_htmlReader) {
        return;
    }
	if (event->button() == Qt::LeftButton) {
		this->setFocus();
	} else if (event->button() == Qt::RightButton) {
		QMenu *menu = new QMenu(this);
		QAction *ac = new QAction(tr("Go to HTML"), menu);
		menu->addAction(ac);
		connect(ac, SIGNAL(triggered()), this, SLOT(gobackToHTMLOffset()));
		menu->exec(event->globalPos());
		delete menu;
	}
}

// close event
void PreviewHTMLWindow::closeEvent(QCloseEvent *)
{
	cleanResource();
}

// keyboard press
void PreviewHTMLWindow::keyPressEvent(QKeyEvent *event)
{
    if (!m_htmlReader) {
        return;
    }
	if (event->key() == Qt::Key_Left) {
        if (m_currentPageIndex == 0) {
            return;
        }
        LayoutEngine::GetEngine()->gotoPreviousPage(m_htmlReader);
        m_currentPageIndex -= 1;
	} else if (event->key() == Qt::Key_Right) {
        if (m_currentPageIndex == LayoutEngine::GetEngine()->getPageCount(m_htmlReader)) {
            return;
        }
        LayoutEngine::GetEngine()->gotoNextPage(m_htmlReader);
        m_currentPageIndex += 1;
	}
    m_viewCore->updateView();
}

void PreviewHTMLWindow::closed()
{
	closeEvent(nullptr);
	cleanTempFile();
}

QSize PreviewHTMLWindow::sizeHint() const
{
	return m_standardSize;
}

QSize PreviewHTMLWindow::minimumSizeHint() const
{
	return QSize(m_standardSize.width() * 0.75, m_standardSize.height() * 0.75);
}

void PreviewHTMLWindow::reloadHTML(std::string htmlPath, bool reload, const QSize& standardSize)
{
    if (!reload) {
        return;
    }
    cleanResource();
    initialCoreView();
    m_htmlPath = htmlPath;
    if (standardSize.width() != 0 || standardSize.height() != 0) {
        m_standardSize = standardSize;
    }
    
    QFile f(htmlPath.c_str());
    m_htmlPath = htmlPath;
    m_innerHtmlPath = tempFilePath(m_htmlPath);
    if ( !QFile(tempFilePath(m_htmlPath).c_str()).exists() ) {
        f.copy(tempFilePath(m_htmlPath).c_str());
    }
    LayoutEngine::GetEngine()->openHtml(m_viewCore, m_innerHtmlPath, "html_id_key", [this](HTMLReader* reader, int ecode) {
        engineOpenHTMLFinish(reader, ecode);
    });
}

void PreviewHTMLWindow::updateCurrentPage(const QString& contentTexts)
{
    cleanResource();
    QFile f(m_innerHtmlPath.c_str());
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(contentTexts.toUtf8());
        f.close();
    }
    LayoutEngine::GetEngine()->openHtml(m_viewCore, m_innerHtmlPath, "html_id_key", [this](HTMLReader* reader, int ecode) {
        engineOpenHTMLFinish(reader, ecode);
    });
}

void PreviewHTMLWindow::updateForOffset(unsigned int htmlOffset)
{
    if ( !m_htmlReader ) {
        return;
    }
    refreshView();
}

void PreviewHTMLWindow::cleanTempFile()
{
    QDir dir(QFileInfo(m_htmlPath.c_str()).absolutePath());
    QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::NoSymLinks);
    for (QFileInfo f_info : file_list) {
        if (f_info.fileName().startsWith("__Temp__")) {
            QFile(f_info.filePath()).remove();
        }
    }
}

void PreviewHTMLWindow::cleanResource()
{
    if (m_htmlReader) {
        LayoutEngine::GetEngine()->closeHtml(m_htmlReader);
        m_htmlReader = 0;
    }
    if (m_viewCore) {
        delete m_viewCore;
        m_viewCore = 0;
    }
    m_currentPageIndex = 0;
    m_isNightMode = false;
}

void PreviewHTMLWindow::refreshView() const
{
	if (!LayoutEngine::GetEngine() && !m_htmlReader) {
		return;
	}
}

std::string PreviewHTMLWindow::tempFilePath(const std::string & ofilePath)
{
	if (ofilePath.empty()) {
		return "";
	}

	QFileInfo oinfo(ofilePath.c_str());
	std::string fileName = "__Temp__" + oinfo.fileName().toStdString();
	std::string dir = oinfo.absolutePath().toStdString() + "/";
	if (fileName.empty()) {
		return "";
	}
	return dir + fileName;
}

void PreviewHTMLWindow::gobackToHTMLOffset() {
	if (!m_htmlReader) return;
    int offset = LayoutEngine::GetEngine()->getStartHTMLOffsetForPageIndex(m_htmlReader, m_currentPageIndex);
	if (offset < 0) return;
	// emit signal
	emit mapbackToHtml(offset);
}

void PreviewHTMLWindow::bgColorChange(int idx, bool isNightMode)
{
	if ( !isVisible() && idx >= m_supportedBGColorCnt ) {
		return;
	}
	std::string color = m_supportBGColor[idx];
	if (color.empty()) {
		return;
	}
	if (m_isNightMode != isNightMode) {
		m_isNightMode = isNightMode;
		LayoutEngine::GetEngine()->setIsNightMode(isNightMode);
        LayoutEngine::GetEngine()->repaint(m_htmlReader);
	}
	std::string prop = "background-color:";
	setStyleSheet((prop + color).c_str());
}

void PreviewHTMLWindow::initialCoreView()
{
    if (m_viewCore) {
        return;
    }
    m_viewCore = new HTMLViewQt(this);
    m_viewCore->setUpdateViewCallback([this]() {
        update();
    });
}
