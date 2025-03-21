#include <QApplication>
#include <QCamera>
#include <QDebug>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QPermission>
#include <QPixmap>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QVideoSink>
#include <QWidget>
#include <QWindow>
#include <QtMultimediaWidgets/QVideoWidget>
#include <cstdint>
#include <exception>

#include "ch9329.h"

constexpr size_t write_wait_msec = 400;

struct Pkt {
  uint8_t head[2];
  uint8_t addr;
  uint8_t cmd;
  uint8_t len;
  uint8_t modifier;
  uint8_t padding1;
  uint8_t code;
  uint8_t padding2[5];
  uint8_t sum;

  static constexpr uint8_t CMD_KEYBOARD = 0x02;
  static constexpr uint8_t CMD_KEYBOARD_MULTI = 0x03;
  static constexpr uint8_t CMD_MOUSE_ABS = 0x04;
  static constexpr uint8_t CMD_MOUSE_REL = 0x05;

  void set_header() {
    head[0] = 0x57;
    head[1] = 0xab;
  }

  void set_modifier(bool lctrl, bool lshift, bool lalt, bool lwin, bool rctl,
                    bool rshift, bool ralt, bool rwin) {
    modifier = 0;
    if (lctrl)
      modifier |= 0b1;
    if (lshift)
      modifier |= 0b10;
    if (lalt)
      modifier |= 0b100;
    if (lwin)
      modifier |= 0b1000;
    if (rctl)
      modifier |= 0b10000;
    if (rshift)
      modifier |= 0b100000;
    if (ralt)
      modifier |= 0b1000000;
    if (rwin)
      modifier |= 0b10000000;
  }

  void set_key_down() {
    addr = 0;
    cmd = CMD_KEYBOARD;
    len = 8;
  }
  void set_key(uint8_t code) { this->code = code; }

  void calc_sum() {
    size_t sz = sizeof(*this) - 1;
    uint8_t *ptr = (uint8_t *)this;
    sum = 0;
    for (size_t i = 0; i < sz; ++i) {
      sum += ptr[i];
    }
  }

  void make_key_down(uint8_t code, bool lctrl, bool lshift, bool lalt,
                     bool lwin, bool rctl, bool rshift, bool ralt, bool rwin) {
    memset(this, 0, sizeof(*this));
    set_header();
    set_key(code);
    set_modifier(lctrl, lshift, lalt, lwin, rctl, rshift, ralt, rwin);
    set_key_down();
    calc_sum();
  }
  void make_key_up() {
    make_key_down(0, false, false, false, false, false, false, false, false);
  }

} __attribute__((packed));

uint8_t key_to_scancode(uint32_t key);
void key_to_scancode_init();

class MainWidget : public QVideoWidget {
public:
  MainWidget(QString port_name) {

    // setMinimumWidth(640);
    // setMinimumHeight(480);
    // resize(640, 480);
    init_perm();
    init_serial(port_name);
  }

  virtual ~MainWidget() { serial.close(); }

protected:
  void keyPressEvent(QKeyEvent *event) override {

    uint32_t virtkey = event->nativeVirtualKey();
    uint8_t code = key_to_scancode(virtkey);
    if (code) {
      pkt.make_key_down(code, ctrl_left, shift_left, alt_left, win_left,
                        ctrl_right, shift_right, alt_right, win_right);
      write_pkt();
      pkt.make_key_up();
      write_pkt();
    }
    switch (code) {
    case CH9_Control:
      ctrl_left = true;
      break;
    case CH9_Shift:
      shift_left = true;
      break;
    case CH9_Alt:
      alt_left = true;
      break;
    case CH9_Win:
      win_left = true;
      break;
    case CH9_RightControl:
      ctrl_right = true;
      break;
    case CH9_RightShift:
      shift_right = true;
      break;
    case CH9_RightAlt:
      alt_right = true;
      break;
    case CH9_RightWin:
      win_right = true;
      break;
    }

    QVideoWidget::keyPressEvent(event);
  }

  void keyReleaseEvent(QKeyEvent *event) override {
    uint32_t virtkey = event->nativeVirtualKey();
    uint8_t code = key_to_scancode(virtkey);
    switch (code) {
    case CH9_Control:
      ctrl_left = false;
      break;
    case CH9_Shift:
      shift_left = false;
      break;
    case CH9_Alt:
      alt_left = false;
      break;
    case CH9_Win:
      win_left = false;
      break;
    case CH9_RightControl:
      ctrl_right = false;
      break;
    case CH9_RightShift:
      shift_right = false;
      break;
    case CH9_RightAlt:
      alt_right = false;
      break;
    case CH9_RightWin:
      win_right = false;
      break;
    }
    QVideoWidget::keyReleaseEvent(event);
  }

private:
  QSerialPort serial;
  Pkt pkt;
  void write_pkt() {
    serial.write((const char *)&pkt, sizeof(pkt));
    if (serial.waitForBytesWritten(write_wait_msec)) {
      qDebug() << "Data written successfully.";
    } else {
      qDebug() << "Error writing data:" << serial.errorString();
    }
  }

  bool ctrl_left = false;
  bool shift_left = false;
  bool alt_left = false;
  bool win_left = false;
  bool ctrl_right = false;
  bool shift_right = false;
  bool alt_right = false;
  bool win_right = false;

  QScopedPointer<QCamera> m_camera;
  QMediaCaptureSession m_captureSession;

  QWidget *container;
  QVBoxLayout *layout;
  QVideoWidget *viewfinder;

  void init_serial(QString port_name) {
    serial.setPortName(port_name);
    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    if (!serial.open(QIODevice::ReadWrite)) {
      qDebug() << "Failed to open port:" << serial.errorString();
      throw std::exception();
    }
  }

  void init_cam() {

    auto cam = QMediaDevices::defaultVideoInput();
    m_camera.reset(new QCamera(cam));
    m_captureSession.setCamera(m_camera.data());

    // viewfinder = new QVideoWidget(container);
    m_captureSession.setVideoOutput(this);

    // layout = new QVBoxLayout(container);
    // layout->setContentsMargins(0, 0, 0, 0); // Remove margins
    // layout->setSpacing(0);                  // Remove spacing
    // layout->addWidget(viewfinder);

    // container->show();

    m_camera->start();
    qDebug() << "connect cam";
    // Connect to the videoFrameChanged signal to update the video label
  }

  void init_perm() {
    QCameraPermission cameraPermission;
    switch (qApp->checkPermission(cameraPermission)) {
    case Qt::PermissionStatus::Undetermined:
      qApp->requestPermission(cameraPermission, this, &MainWidget::init_cam);
      return;
    case Qt::PermissionStatus::Denied:
      qWarning("Camera permission is not granted!");
      return;
    case Qt::PermissionStatus::Granted:
      break;
    }
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  const auto ports = QSerialPortInfo::availablePorts();
  qDebug() << "Available serial ports:";
  QVector<QSerialPortInfo> usb_ports;
  for (const QSerialPortInfo &port : ports) {
    auto port_name = port.portName();

    if (port_name.startsWith("tty.usbserial-")) {
      usb_ports.emplace_back(port);
    }
  }

  for (auto &usb_port : usb_ports) {

    qDebug() << "Port:" << usb_port.portName()
             << "Description:" << usb_port.description();
  }

  if (usb_ports.size() == 0) {
    qDebug() << "No USB serial port found";
    return 1;
  }
  auto &usb_port = usb_ports[0];
  key_to_scancode_init();
  MainWidget main_widget(usb_port.portName());
  main_widget.resize(640, 480);
  main_widget.show();
  return app.exec();
}
