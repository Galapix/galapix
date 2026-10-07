// SPDX-FileCopyrightText: 2005-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstinput/wiimote.hpp>

#include <logmich/log.hpp>

namespace wstinput {

#ifdef HAVE_CWIID

namespace {

Wiimote* wiimote = nullptr;

} // namespace

void
Wiimote::init()
{
  wiimote = new Wiimote();
}

void
Wiimote::deinit()
{
  delete wiimote;
}

Wiimote::Wiimote()
  : mutex(),
    m_wiimote(0),
    m_rumble(false),
    m_led_state(0),
    m_nunchuk_btns(0),
    m_nunchuk_stick_x(0),
    m_nunchuk_stick_y(0),
    m_buttons(0),
    wiimote_zero(),
    wiimote_one(),
    nunchuk_zero(),
    nunchuk_one(),
    events()
{
  pthread_mutex_init(&mutex, NULL);

  assert(wiimote == 0);
  wiimote = this;

  cwiid_set_err(&Wiimote::err_callback);
}

Wiimote::~Wiimote()
{
  disconnect();
  pthread_mutex_destroy(&mutex);
}

void
Wiimote::connect()
{
  assert(m_wiimote == 0);

  /* Connect to any wiimote */
  bdaddr_t bdaddr = {{0, 0, 0, 0, 0, 0}}; // BDADDR_ANY

  /* Connect to address in string WIIMOTE_BDADDR */
  /* str2ba(WIIMOTE_BDADDR, &bdaddr); */

  /* Connect to the wiimote */
  printf("Put Wiimote in discoverable mode now (press 1+2)...\n");

  if (!(m_wiimote = cwiid_connect(&bdaddr, CWIID_FLAG_MESG_IFC)))
  {
    log_error("Unable to connect to wiimote");
  }
  else
  {
    log_debug("Wiimote connected: {}", m_wiimote);
    if (cwiid_set_mesg_callback(m_wiimote, &Wiimote::mesg_callback)) {
      log_error("Unable to set message callback");
    }

    if (cwiid_command(m_wiimote, CWIID_CMD_RPT_MODE,
                      CWIID_RPT_STATUS  |
                      CWIID_RPT_NUNCHUK |
                      CWIID_RPT_ACC     |
                      CWIID_RPT_BTN))
    {
      log_error("Wiimote: Error setting report mode");
    }

    { // read calibration data
      uint8_t buf[7];

      if (cwiid_read(m_wiimote, CWIID_RW_EEPROM, 0x16, 7, buf))
      {
        log_error("Wiimote: Unable to retrieve accelerometer calibration");
      }
      else
      {
        wiimote_zero.x = buf[0];
        wiimote_zero.y = buf[1];
        wiimote_zero.z = buf[2];

        wiimote_one.x  = buf[4];
        wiimote_one.y  = buf[5];
        wiimote_one.z  = buf[6];
      }

      if (cwiid_read(m_wiimote, CWIID_RW_REG | CWIID_RW_DECODE, 0xA40020, 7, buf))
      {
        log_error("Wiimote: Unable to retrieve wiimote accelerometer calibration");
      }
      else
      {
        nunchuk_zero.x = buf[0];
        nunchuk_zero.y = buf[1];
        nunchuk_zero.z = buf[2];

        nunchuk_one.x  = buf[4];
        nunchuk_one.y  = buf[5];
        nunchuk_one.z  = buf[6];
      }

      log_debug("Wiimote Calibration: {}, {}, {} - {}, {}, {}",
                static_cast<int>(wiimote_zero.x),
                static_cast<int>(wiimote_zero.x),
                static_cast<int>(wiimote_zero.x),
                static_cast<int>(wiimote_one.x),
                static_cast<int>(wiimote_one.x),
                static_cast<int>(wiimote_one.x));

      log_debug("Nunchuk Calibration: {}, {}, {} - {}, {}, {}",
                static_cast<int>(nunchuk_zero.x),
                static_cast<int>(nunchuk_zero.x),
                static_cast<int>(nunchuk_zero.x),
                static_cast<int>(nunchuk_one.x),
                static_cast<int>(nunchuk_one.x),
                static_cast<int>(nunchuk_one.x));
    }
  }
}

void
Wiimote::disconnect()
{
  if (m_wiimote)
  {
    cwiid_disconnect(m_wiimote);
    m_wiimote = 0;
  }
}

void
Wiimote::set_led(unsigned char led_state)
{
  if (m_led_state != led_state)
  {
    m_led_state = led_state;

    if (cwiid_command(m_wiimote, CWIID_CMD_LED, m_led_state)) {
      log_error("Error setting LEDs");
    }
  }
}

void
Wiimote::set_led(int num, bool state)
{
  assert(num >= 1 && num <= 4);

  unsigned int new_led_state = m_led_state;
  if (state)
    new_led_state |= (1u << (num-1));
  else // (!state)
    new_led_state &= ~(1u << (num-1));

  set_led(static_cast<unsigned char>(new_led_state));
}

void
Wiimote::set_rumble(bool r)
{
  if (r != m_rumble)
  {
    m_rumble = r;

    if (cwiid_command(m_wiimote, CWIID_CMD_RUMBLE, m_rumble)) {
      log_error("Error setting rumble");
    }
  }
}

void
Wiimote::add_button_event(int /*device*/, int button, bool down)
{
  WiimoteEvent event;

  event.type = WiimoteEvent::WIIMOTE_BUTTON_EVENT;
  event.button.device = 0;
  event.button.button = button;
  event.button.down   = down;

  events.push_back(event);
}

void
Wiimote::add_axis_event(int /*device*/, int axis, float pos)
{
  WiimoteEvent event;

  event.type = WiimoteEvent::WIIMOTE_AXIS_EVENT;
  event.axis.device = 0;
  event.axis.axis = axis;
  event.axis.pos  = pos;

  events.push_back(event);
}

void
Wiimote::add_acc_event(int /*device*/, int accelerometer, float x, float y, float z)
{
  WiimoteEvent event;

  event.type = WiimoteEvent::WIIMOTE_ACC_EVENT;
  event.acc.device = 0;
  event.acc.accelerometer = accelerometer;
  event.acc.x = x;
  event.acc.y = y;
  event.acc.z = z;

  events.push_back(event);
}


void
Wiimote::on_status(const cwiid_status_mesg& msg)
{
  printf("Status Report: battery=%d extension=", msg.battery);
  switch (msg.ext_type)
  {
    case CWIID_EXT_NONE:
      printf("none");
      break;

    case CWIID_EXT_NUNCHUK:
      printf("Nunchuk");
      break;

    case CWIID_EXT_CLASSIC:
      printf("Classic Controller");
      break;

    default:
      printf("Unknown Extension");
      break;
  }
  printf("\n");
}

void
Wiimote::on_error(const cwiid_error_mesg& /*msg*/)
{
  log_error("On Error");

  if (m_wiimote)
  {
    if (cwiid_disconnect(m_wiimote))
    {
      log_error("Error on wiimote disconnect");
      m_wiimote = 0;
    }
  }
}

void
Wiimote::on_button(const cwiid_btn_mesg& msg)
{
#define CHECK_BTN(btn, num) if (changes & btn) add_button_event(0, num, m_buttons & btn)

  uint16_t changes = static_cast<uint16_t>(m_buttons ^ msg.buttons);
  m_buttons = msg.buttons;

  CHECK_BTN(CWIID_BTN_A, 0);
  CHECK_BTN(CWIID_BTN_B, 1);

  CHECK_BTN(CWIID_BTN_LEFT,  2);
  CHECK_BTN(CWIID_BTN_RIGHT, 3);
  CHECK_BTN(CWIID_BTN_UP,    4);
  CHECK_BTN(CWIID_BTN_DOWN,  5);

  CHECK_BTN(CWIID_BTN_PLUS,  6);
  CHECK_BTN(CWIID_BTN_HOME,  7);
  CHECK_BTN(CWIID_BTN_MINUS, 8);

  CHECK_BTN(CWIID_BTN_1,  9);
  CHECK_BTN(CWIID_BTN_2, 10);
}

void
Wiimote::on_acc(const cwiid_acc_mesg& msg)
{
  //printf("Acc Report: x=%d, y=%d, z=%d\n", msg.acc[0], msg.acc[1], msg.acc[2]);

  add_acc_event(0, 0,
                static_cast<float>(msg.acc[0] - wiimote_zero.x) / static_cast<float>(wiimote_one.x - wiimote_zero.x),
                static_cast<float>(msg.acc[1] - wiimote_zero.y) / static_cast<float>(wiimote_one.y - wiimote_zero.y),
                static_cast<float>(msg.acc[2] - wiimote_zero.z) / static_cast<float>(wiimote_one.z - wiimote_zero.z));
}

void
Wiimote::on_ir(const cwiid_ir_mesg& msg)
{
  printf("IR Report: ");
  for (int i = 0; i < CWIID_IR_SRC_COUNT; ++i)
  {
    if (msg.src[i].valid) {
      printf("(%d,%d) ", msg.src[i].pos[0], msg.src[i].pos[1]);
    }
  }
}

/** Convert value to float while taking calibration data, left/center/right into account */
inline float to_float(uint8_t min,
                      uint8_t center,
                      uint8_t max,
                      uint8_t value)
{
  if (value < center)
  {
    return math::mid(-1.0f, -static_cast<float>(center - value) / static_cast<float>(center - min), 1.0f);
  }
  else if (value > center)
  {
    return math::mid(-1.0f, static_cast<float>(value - center) / static_cast<float>(max - center), 1.0f);
  }
  else
  {
    return 0.0f;
  }
}

void
Wiimote::on_nunchuck(const cwiid_nunchuk_mesg& msg)
{
  uint8_t changes = static_cast<uint8_t>(m_nunchuk_btns ^ msg.buttons);
  m_nunchuk_btns  = msg.buttons;

#define CHECK_NCK_BTN(btn, num) if (changes & btn) add_button_event(0, num, m_nunchuk_btns & btn)

  CHECK_NCK_BTN(CWIID_NUNCHUK_BTN_Z, 11);
  CHECK_NCK_BTN(CWIID_NUNCHUK_BTN_C, 12);


  // FIXME: Read real calibration data, instead of hardcoded one
  float nunchuk_stick_x =  to_float(37, 129, 231, msg.stick[0]);
  float nunchuk_stick_y = -to_float(22, 119, 213, msg.stick[1]);

  if (m_nunchuk_stick_x != nunchuk_stick_x)
  {
    m_nunchuk_stick_x = nunchuk_stick_x;
    add_axis_event(0, 0, m_nunchuk_stick_x);
  }

  if (m_nunchuk_stick_y != nunchuk_stick_y)
  {
    m_nunchuk_stick_y = nunchuk_stick_y;
    add_axis_event(0, 1, m_nunchuk_stick_y);
  }

  add_acc_event(0, 1,
                static_cast<float>(msg.acc[0] - nunchuk_zero.x) / static_cast<float>(nunchuk_one.x - nunchuk_zero.x),
                static_cast<float>(msg.acc[1] - nunchuk_zero.y) / static_cast<float>(nunchuk_one.y - nunchuk_zero.y),
                static_cast<float>(msg.acc[2] - nunchuk_zero.z) / static_cast<float>(nunchuk_one.z - nunchuk_zero.z));
  if (0)
    printf("Nunchuk Report: btns=%.2X stick=(%3d,%3d) (%5.2f, %5.2f) acc.x=%d acc.y=%d acc.z=%d\n",
           msg.buttons,
           msg.stick[0], msg.stick[1],
           m_nunchuk_stick_x,
           m_nunchuk_stick_y,
           msg.acc[0], msg.acc[1], msg.acc[2]);
}

void
Wiimote::on_classic(const cwiid_classic_mesg& msg)
{
  printf("Classic Report: btns=%.4X l_stick=(%d,%d) r_stick=(%d,%d) "
         "l=%d r=%d\n", msg.buttons,
         msg.l_stick[0], msg.l_stick[1],
         msg.r_stick[0], msg.r_stick[1],
         msg.l, msg.r);
}

std::vector<WiimoteEvent>
Wiimote::pop_events()
{
  pthread_mutex_lock(&mutex);
  std::vector<WiimoteEvent> ret = events;
  events.clear();
  pthread_mutex_unlock(&mutex);
  return ret;
}

// Callback function that get called by the Wiimote thread
void
Wiimote::err(cwiid_wiimote_t* w, const char *s, va_list ap)
{
  pthread_mutex_lock(&mutex);

  if (w)
    printf("%d:", cwiid_get_id(w));
  else
    printf("-1:");

  vprintf(s, ap);
  printf("\n");

  pthread_mutex_unlock(&mutex);
}

void
Wiimote::mesg(cwiid_wiimote_t* /*w*/, int mesg_count, union cwiid_mesg msg[])
{
  pthread_mutex_lock(&mutex);

  for (int i=0; i < mesg_count; i++)
  {
    switch (msg[i].type)
    {
      case CWIID_MESG_STATUS:
        wiimote->on_status(msg[i].status_mesg);
        break;

      case CWIID_MESG_BTN:
        wiimote->on_button(msg[i].btn_mesg);
        break;

      case CWIID_MESG_ACC:
        wiimote->on_acc(msg[i].acc_mesg);
        break;

      case CWIID_MESG_IR:
        wiimote->on_ir(msg[i].ir_mesg);
        break;

      case CWIID_MESG_NUNCHUK:
        wiimote->on_nunchuck(msg[i].nunchuk_mesg);
        break;

      case CWIID_MESG_CLASSIC:
        wiimote->on_classic(msg[i].classic_mesg);
        break;

      case CWIID_MESG_ERROR:
        wiimote->on_error(msg[i].error_mesg);
        break;

      default:
        printf("Wiimote: Unknown Report");
        break;
    }
  }

  pthread_mutex_unlock(&mutex);
}

// static callback functions

void
Wiimote::err_callback(cwiid_wiimote_t* w, const char *s, va_list ap)
{
  wiimote->err(w, s, ap);
}

void
Wiimote::mesg_callback(cwiid_wiimote_t* w, int mesg_count, union cwiid_mesg mesg[], timespec*)
{
  wiimote->mesg(w, mesg_count, mesg);
}

#endif // HAVE_CWIID

} // namespace wstinput

/* EOF */
