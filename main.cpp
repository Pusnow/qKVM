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

struct PktHeader {
  uint8_t head[2];
  uint8_t addr;
  uint8_t cmd;
  uint8_t len;

  static constexpr uint8_t CMD_KEYBOARD = 0x02;
  static constexpr uint8_t CMD_KEYBOARD_MULTI = 0x03;
  static constexpr uint8_t CMD_MOUSE_ABS = 0x04;
  static constexpr uint8_t CMD_MOUSE_REL = 0x05;

  void set_header() {
    head[0] = 0x57;
    head[1] = 0xab;
  }

  void set_keybd() {
    addr = 0;
    cmd = CMD_KEYBOARD;
    len = 8;
  }

  void set_mouse() {
    addr = 0;
    cmd = CMD_MOUSE_ABS;
    len = 7;
  }

} __attribute__((packed));

struct PktKbd {
  PktHeader head;
  uint8_t modifier;
  uint8_t padding1;
  uint8_t code;
  uint8_t padding2[5];
  uint8_t sum;

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
    head.set_header();
    set_key(code);
    set_modifier(lctrl, lshift, lalt, lwin, rctl, rshift, ralt, rwin);
    head.set_keybd();
    calc_sum();
  }
  void make_key_up() {
    make_key_down(0, false, false, false, false, false, false, false, false);
  }

} __attribute__((packed));

struct PktMouse {
  PktHeader head;
  uint8_t fixed_2;
  uint8_t button;
  uint16_t x;
  uint16_t y;
  uint8_t scroll;
  uint8_t sum;

  void calc_sum() {
    size_t sz = sizeof(*this) - 1;
    uint8_t *ptr = (uint8_t *)this;
    sum = 0;
    for (size_t i = 0; i < sz; ++i) {
      sum += ptr[i];
    }
  }

  void make_move(float x_f, float y_f) {
    memset(this, 0, sizeof(*this));
    head.set_header();
    head.set_mouse();

    fixed_2 = 0x2;

    uint16_t x_int = 4096 * x_f;
    uint16_t y_int = 4096 * y_f;
    x = x_int;
    y = y_int;

    calc_sum();
  }

} __attribute__((packed));

uint8_t key_to_scancode(uint32_t key);
void key_to_scancode_init();

class MainWidget : public QVideoWidget {
public:
  MainWidget(QString port_name) {

    init_perm();
    init_serial(port_name);
  }

  virtual ~MainWidget() { serial.close(); }

protected:
  void keyPressEvent(QKeyEvent *event) override {

    uint32_t virtkey = event->nativeVirtualKey();
    uint8_t code = key_to_scancode(virtkey);
    if (code) {
      pkt_kbd.make_key_down(code, ctrl_left, shift_left, alt_left, win_left,
                            ctrl_right, shift_right, alt_right, win_right);
      write_pkt_kbd();
      pkt_kbd.make_key_up();
      write_pkt_kbd();
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

  void mousePressEvent(QMouseEvent *event) override {
    int w_widget = width();
    int h_widget = height();

    auto size_hint = sizeHint();

    int w_cam = size_hint.width();
    int h_cam = size_hint.height();

    int h_widget_calc = w_widget * h_cam / w_cam;
    int w_widget_calc = h_widget * w_cam / h_cam;

    QPointF localPos = event->position();

    int w_press = localPos.x();
    int h_press = localPos.y();

    float x = 0;
    float y = 0;

    qDebug() << "w_widget: " << w_widget << " w_cam: " << w_cam
             << " w_press: " << w_press << " w_widget_calc: " << w_widget_calc
             << "h_widget: " << h_widget << " h_cam: " << h_cam
             << " h_press: " << h_press << " h_widget_calc: " << h_widget_calc;

    // w_widget:  640  w_cam:  1920  w_press:  248  w_widget_calc:  853
    // h_widget:  480  h_cam:  1080  h_press:  189  h_widget_calc:  360
    if (h_widget > h_widget_calc) {
      // top/bottom letterbox
      int letter_size = (h_widget - h_widget_calc) / 2;

      if (h_press < letter_size || h_press > h_widget - letter_size) {
        QVideoWidget::mousePressEvent(event);
        return;
      }

      x = 1.0f * w_press / w_widget;
      y = 1.0f * (h_press - letter_size) / h_widget_calc;

    } else {
      // left/right letterbox
      int letter_size = (w_widget - w_widget_calc) / 2;

      if (w_press < letter_size || w_press > w_widget - letter_size) {
        QVideoWidget::mousePressEvent(event);
        return;
      }

      x = 1.0f * (w_press - letter_size) / w_widget_calc;
      y = 1.0f * h_press / h_widget;
    }

    // QPointF localPos = event->position();
    // auto button = event->button();

    pkt_mouse.make_move(x, y);
    write_pkt_mouse();

    // qDebug() << "button: " << event->button() << " pos: " << localPos;

    // Pass other mouse events to the base class implementation.
    QVideoWidget::mousePressEvent(event);
  }

  // void mouseMoveEvent(QMouseEvent *e) override {
  //   qDebug("move: %d, %d", e->pos().x(), e->pos().y());
  //   QVideoWidget::mouseMoveEvent(e);
  // }

private:
  QSerialPort serial;
  PktKbd pkt_kbd;
  PktMouse pkt_mouse;

  void write_pkt_kbd() {
    serial.write((const char *)&pkt_kbd, sizeof(pkt_kbd));
    if (serial.waitForBytesWritten(write_wait_msec)) {
      qDebug() << "Data written successfully.";
    } else {
      qDebug() << "Error writing data:" << serial.errorString();
    }
  }

  void write_pkt_mouse() {
    serial.write((const char *)&pkt_mouse, sizeof(pkt_mouse));
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
    m_captureSession.setVideoOutput(this);

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
