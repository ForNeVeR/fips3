#include <memory>

#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QMessageBox>

#include <application.h>
#include <mainwindow.h>

MainWindow::Exception::Exception(const QString& what):
	::Exception(what) {
}
void MainWindow::Exception::raise() const {
	throw *this;
}
QException* MainWindow::Exception::clone() const {
	return new MainWindow::Exception(*this);
}

MainWindow::FileOpenError::FileOpenError(const QString& reason):
	MainWindow::Exception(reason) {
}
void MainWindow::FileOpenError::raise() const {
	throw *this;
}
QException* MainWindow::FileOpenError::clone() const {
	return new MainWindow::FileOpenError(*this);
}
MainWindow::NoImageInFITS::NoImageInFITS():
	MainWindow::Exception("The file has no image content") {
}
void MainWindow::NoImageInFITS::raise() const {
	throw *this;
}
QException* MainWindow::NoImageInFITS::clone() const {
	return new MainWindow::NoImageInFITS(*this);
}

MainWindow::MainWindow(const QString& fits_filename, QWidget *parent): QMainWindow(parent) {
	// Open FITS file
	std::unique_ptr<QFile> file{new QFile(fits_filename)};
	if (!file->open(QIODevice::ReadOnly)) {
		throw FileOpenError(file->errorString());
	}

	// Read FITS from file
	fits_.reset(new FITS(file.release()));
	const FITS::HeaderDataUnit* hdu = &fits_->primary_hdu();

	for (auto it = fits_->begin();
		it != fits_->end() && !hdu->data().imageDataUnit();
		++it) {

		hdu = &(*it);
	}

	if (!hdu->data().imageDataUnit()) {
		throw NoImageInFITS();
	}

	// Resize window to fit FITS image
	const auto desktop_size = QApplication::desktop()->screenGeometry();
	const QSize maximum_initial_window_size(desktop_size.width() * 2 / 3, desktop_size.height() * 2 / 3);
	resize(hdu->data().imageDataUnit()->size().boundedTo(maximum_initial_window_size));

	#ifdef Q_OS_MAC
		setWindowTitle(QFileInfo(fits_filename).fileName());
	#else
		setWindowTitle(QFileInfo(fits_filename).fileName() + " — FIPS");
	#endif

	// Create scroll area and put there open_gl_widget
	std::unique_ptr<ScrollZoomArea> scroll_zoom_area{new ScrollZoomArea(this, *hdu)};
	/* setCentralWidget promises to take ownership */
	setCentralWidget(scroll_zoom_area.release());

	std::unique_ptr<QMenuBar> menu_bar{new QMenuBar(this)};
	// File menu
	auto file_menu = menu_bar->addMenu(tr("&File"));
	auto file_open_action = file_menu->addAction(tr("&Open"), Application::instance(), SLOT(openFile(void)));
	file_open_action->setShortcut(QKeySequence::Open);
	auto file_close_action = file_menu->addAction(tr("&Close"), this, SLOT(close()));
	file_close_action->setShortcut(QKeySequence::Close);
	// View menu
	auto view_menu = menu_bar->addMenu(tr("&View"));
	auto zoomIn_action = view_menu->addAction(tr("Zoom &In"), this, SLOT(zoomIn(void)));
	zoomIn_action->setShortcut(QKeySequence::ZoomIn);
	auto zoomOut_action = view_menu->addAction(tr("Zoom &Out"), this, SLOT(zoomOut(void)));
	zoomOut_action->setShortcut(QKeySequence::ZoomOut);
	auto fit_to_window_action = view_menu->addAction(tr("&Fit to Window"), this, SLOT(fitToWindow(void)));
	fit_to_window_action->setShortcut(tr("Ctrl+F"));
	view_menu->addSeparator();
	// To be continued in docks block
	// Help menu
	auto help_menu = menu_bar->addMenu(tr("&Help"));
	help_menu->addAction(tr("&About"), this, SLOT(about()));
	help_menu->addAction(tr("&Homepage"), this, SLOT(homepage()));
	/* setMenuBar promises to take ownership */
	setMenuBar(menu_bar.release());

	std::unique_ptr<QDockWidget> levels_dock{new QDockWidget(tr("Levels"), this)};
	levels_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	view_menu->addAction(levels_dock->toggleViewAction());
	levels_dock->toggleViewAction()->setShortcut(tr("Ctrl+L"));
	std::unique_ptr<LevelsWidget> levels_widget{new LevelsWidget(levels_dock.get())};
	connect(
			scrollZoomArea()->viewport(), SIGNAL(textureInitialized(const OpenGLTexture*)),
			levels_widget.get(), SLOT(notifyTextureInitialized(const OpenGLTexture*))
	);
	connect(
			levels_widget.get(), SIGNAL(valuesChanged(const std::pair<double, double>&)),
			scrollZoomArea()->viewport(), SLOT(changeLevels(const std::pair<double, double>&))
	);
	levels_dock->setWidget(levels_widget.release());
	addDockWidget(Qt::RightDockWidgetArea, levels_dock.release());

	std::unique_ptr<QDockWidget> colormap_dock{new QDockWidget(tr("Color map"), this)};
	colormap_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	view_menu->addAction(colormap_dock->toggleViewAction());
	colormap_dock->toggleViewAction()->setShortcut(tr("Ctrl+E"));
	std::unique_ptr<ColorMapWidget> colormap_widget{new ColorMapWidget(colormap_dock.get(), *scrollZoomArea()->viewport())};
	connect(
			colormap_widget->buttonGroup(), SIGNAL(buttonClicked(int)),
			scrollZoomArea()->viewport(), SLOT(changeColorMap(int))
	);
	colormap_dock->setWidget(colormap_widget.release());
	addDockWidget(Qt::RightDockWidgetArea, colormap_dock.release());
}

void MainWindow::zoomIn() {
	scrollZoomArea()->zoomViewport(zoomIn_factor_);
}

void MainWindow::zoomOut() {
	scrollZoomArea()->zoomViewport(zoomOut_factor_);
}

void MainWindow::fitToWindow() {
	scrollZoomArea()->fitToViewport();
}

void MainWindow::closeEvent(QCloseEvent *event) {
	emit closed(*this);
	QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
	QMainWindow::resizeEvent(event);
}

void MainWindow::about() {
	QMessageBox::about(
			this,
			tr("Fips"),
			tr("<b>Fips.</b> Copyright © 2017 Matwey Kornilov. ") + QString(homePageURL())
	);
}

void MainWindow::homepage() {
	QDesktopServices::openUrl(QUrl(homePageURL()));
}
