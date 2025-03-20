#include <QApplication>
#include <QCamera>
#include <QDebug>
#include <QLabel>
#include <QMediaCaptureSession>
#include <QPixmap>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QVideoSink>
#include <QWidget>
#include <cstdint>

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
    head[0] = 0x57;
    head[1] = 0xab;
    addr = 0;
    cmd = CMD_KEYBOARD;
    len = 8;
  }

  void calc_sum() {
    size_t sz = sizeof(*this) - 1;
    uint8_t *ptr = (uint8_t *)this;
    sum = 0;
    for (size_t i = 0; i < sz; ++i) {
      sum += ptr[i];
    }
  }

} __attribute__((packed));

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

  QSerialPort serial;
  serial.setPortName(
      usb_port.portName()); // Adjust the port name as needed (e.g.,
                            // "/dev/cu.wchusbserialXXXX" on macOS)
  serial.setBaudRate(QSerialPort::Baud9600);
  serial.setDataBits(QSerialPort::Data8);
  serial.setParity(QSerialPort::NoParity);
  serial.setStopBits(QSerialPort::OneStop);
  serial.setFlowControl(QSerialPort::NoFlowControl);
  if (!serial.open(QIODevice::ReadWrite)) {
    qDebug() << "Failed to open port:" << serial.errorString();
    return 1;
  }

  Pkt pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.set_key_in();
  pkt.modifier = 0;
  pkt.code = 0x15;
  pkt.calc_sum();
  serial.write((const char *)&pkt, sizeof(pkt));
  if (serial.waitForBytesWritten(write_wait_msec)) {
    qDebug() << "Data written successfully.";
  } else {
    qDebug() << "Error writing data:" << serial.errorString();
  }
  serial.close();

  return 0;

  // Create a widget to serve as the main window
  QWidget window;
  window.setWindowTitle("Qt6 Webcam View");
  QVBoxLayout layout(&window);

  // QLabel to display video frames
  QLabel videoLabel;
  videoLabel.setMinimumSize(640, 480);
  layout.addWidget(&videoLabel);

  // Create a QCamera object for the default camera
  QCamera camera;

  // Create a media capture session and set the camera
  QMediaCaptureSession captureSession;
  captureSession.setCamera(&camera);

  // Create a QVideoSink to receive video frames
  QVideoSink videoSink;
  captureSession.setVideoSink(&videoSink);

  // Connect to the videoFrameChanged signal to update the video label
  QObject::connect(&videoSink, &QVideoSink::videoFrameChanged,
                   [&](const QVideoFrame &frame) {
                     if (frame.isValid()) {
                       // Convert the video frame to a QImage. Note: this
                       // conversion may require adjustments depending on your
                       // platform and frame format.
                       QImage image = frame.toImage();
                       if (!image.isNull()) {
                         videoLabel.setPixmap(QPixmap::fromImage(image));
                       }
                     }
                   });

  // Start the camera
  camera.start();

  window.show();
  return app.exec();
}
