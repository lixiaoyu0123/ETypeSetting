#pragma once
#include <QWidget>
#include "LayoutEngin.h"
#include "BookChapter.h"
#include <QDockWidget>
#include <memory>

class PreviewHTMLWindow : public QDockWidget, public LayoutEngineDelegate
{
	Q_OBJECT

public:
	PreviewHTMLWindow(QWidget* parent, const std::string htmlPath);
	~PreviewHTMLWindow();
	void reloadHTML(std::string htmlPath, bool reload);
	void updateCurrentPage(const QString& contentTexts);
	void updateForOffset(unsigned int);
	void cleanTempFile();

protected:
	virtual void engineInitFinish();
	virtual void engineOpenBook(BookModel* bookModel, QList<BookContents *>list, LAYOUT_ENGINE_OPEN_EPUB_STATUS error);
	virtual void engineClickResponse(const qint32& originX, const qint32& originY, const QString& chapterId, const qint32& htmlOffset);
	virtual void engineUpdateTotalCount(const qint32& totolPageCount);
	virtual void enginUpdateAllViewPage();
	virtual void engineUpdateVisibleViewPage(EngineHighlightInfo* info);
	virtual void engineShowSelectionMenu(const qint32& x, const qint32& y, const qint32& x2, const qint32& y2);
	virtual void engineOpenUrl(QString urlString);
	virtual void engineClickImage(QImage* image, const qint32& originX, const qint32& originY, const qint32& width, const qint32& height);
	virtual QList<BookUnderlineData *> engineNeedUnderlineData(const QString& charpterId);
	virtual QStringList engineNeedNoteData(const QString& charpterId);
	virtual void enginePaintHighlightRect(const QRect& rect, const QColor& color);
	// only open html file
	virtual void engineOpenHTML(BookChapter *html, LAYOUT_ENGINE_OPEN_EPUB_STATUS error);
	virtual void htmlImageRenderFinish(BookChapter *html, std::shared_ptr<QImage>& pic);

	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *);

	void closeEvent(QCloseEvent *);

	void contextMenuEvent(QContextMenuEvent *);
	void keyPressEvent(QKeyEvent *);


signals:
	void mapbackToHtml(unsigned int);

private slots:
	void gobackToHTMLOffset();

private:
	LayoutEngine *m_engine{ NULL };
	BookChapter* m_htmlModel{ NULL };
	unsigned int m_htmlPageIndex{ 0 };
	unsigned int m_old_htmlPageIndex{ 0 };
	std::string m_htmlPath{ "" };
	std::string m_innerHtmlPath{ "" };
	std::shared_ptr<QImage> m_pic;
	bool m_isRendering{ false };
	QReadWriteLock m_locker;
	QReadWriteLock m_ModelLocker;

private:
	inline void safeSetRenderStatus(bool);
	inline bool safeGetRenderStatus();
	inline bool isRendering() { return safeGetRenderStatus(); };
	inline void setHtmlModel(BookChapter *);
	inline BookChapter* getHtmlModel();
	void cleanResource();
	void refreshView() const;
	std::string tempFilePath(const std::string& fileName);
};
