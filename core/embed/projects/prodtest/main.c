/*
 * This file is part of the Trezor project, https://trezor.io/
 *
 * Copyright (c) SatoshiLabs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <trezor_model.h>
#include <trezor_rtl.h>

#include <gfx/fonts.h>
#include <gfx/gfx_draw.h>
#include <io/display.h>
#include <io/display_utils.h>
#include <io/usb.h>
#include <rtl/cli.h>
#include <sys/system.h>
#include <sys/systick.h>
#include <util/flash.h>
#include <util/flash_otp.h>
#include <util/rsod.h>

#ifdef USE_BUTTON
#include <io/button.h>
#endif

#ifdef USE_SBU
#include <io/sbu.h>
#endif

#ifdef USE_SD_CARD
#include <io/sdcard.h>
#endif

#ifdef USE_TOUCH
#include <io/touch.h>
#endif

#ifdef USE_OPTIGA
#include <sec/optiga_commands.h>
#include <sec/optiga_transport.h>
#include "cmd/prodtest_optiga.h"
#endif

#ifdef USE_HAPTIC
#include <io/haptic.h>
#endif

#ifdef USE_RGB_LED
#include <io/rgb_led.h>
#endif

#ifdef USE_HASH_PROCESSOR
#include <sec/hash_processor.h>
#endif

#ifdef USE_POWERCTL
#include <sys/powerctl.h>
#endif

#ifdef USE_STORAGE_HWKEY
#include <sec/secure_aes.h>
#endif

#ifdef TREZOR_MODEL_T2T1
#define MODEL_IDENTIFIER "TREZOR2-"
#else
#define MODEL_IDENTIFIER MODEL_INTERNAL_NAME "-"
#endif

// Command line interface context
cli_t g_cli = {0};

#define VCP_IFACE 0

static size_t console_read(void *context, char *buf, size_t size) {
  return usb_vcp_read_blocking(VCP_IFACE, (uint8_t *)buf, size, -1);
}

static size_t console_write(void *context, const char *buf, size_t size) {
  return usb_vcp_write_blocking(VCP_IFACE, (const uint8_t *)buf, size, -1);
}

static void vcp_intr(void) { cli_abort(&g_cli); }

static void usb_init_all(void) {
  enum {
    VCP_PACKET_LEN = 64,
    VCP_BUFFER_LEN = 1024,
  };

  static const usb_dev_info_t dev_info = {
      .device_class = 0xEF,     // Composite Device Class
      .device_subclass = 0x02,  // Common Class
      .device_protocol = 0x01,  // Interface Association Descriptor
      .vendor_id = 0x1209,
      .product_id = 0x53C1,
      .release_num = 0x0400,
      .manufacturer = MODEL_USB_MANUFACTURER,
      .product = MODEL_USB_PRODUCT,
      .serial_number = "000000000000",
      .interface = "TREZOR Interface",
      .usb21_enabled = secfalse,
      .usb21_landing = secfalse,
  };

  static uint8_t tx_packet[VCP_PACKET_LEN];
  static uint8_t tx_buffer[VCP_BUFFER_LEN];
  static uint8_t rx_packet[VCP_PACKET_LEN];
  static uint8_t rx_buffer[VCP_BUFFER_LEN];

  static const usb_vcp_info_t vcp_info = {
      .tx_packet = tx_packet,
      .tx_buffer = tx_buffer,
      .rx_packet = rx_packet,
      .rx_buffer = rx_buffer,
      .tx_buffer_len = VCP_BUFFER_LEN,
      .rx_buffer_len = VCP_BUFFER_LEN,
      .rx_intr_fn = vcp_intr,
      .rx_intr_byte = 3,  // Ctrl-C
      .iface_num = VCP_IFACE,
      .data_iface_num = 0x01,
      .ep_cmd = 0x02,
      .ep_in = 0x01,
      .ep_out = 0x01,
      .polling_interval = 10,
      .max_packet_len = VCP_PACKET_LEN,
  };

  ensure(usb_init(&dev_info), NULL);
  ensure(usb_vcp_add(&vcp_info), "usb_vcp_add");
  ensure(usb_start(), NULL);
}

static inline gfx_rect_t gfx_rect_shrink(gfx_rect_t r, int padding) {
  gfx_rect_t result = {
      .x0 = r.x0 + padding,
      .y0 = r.y0 + padding,
      .x1 = r.x1 - padding,
      .y1 = r.y1 - padding,
  };
  return result;
}

static void draw_welcome_screen(void) {
  gfx_clear();
  gfx_rect_t r = gfx_rect_wh(0, 0, DISPLAY_RESX, DISPLAY_RESY);
  uint8_t qr_scale = 4;
  int16_t text_offset = 30;
  gfx_text_attr_t bold = {
      .font = FONT_BOLD,
      .fg_color = COLOR_WHITE,
      .bg_color = COLOR_BLACK,
  };

#if defined TREZOR_MODEL_T2B1 || defined TREZOR_MODEL_T3B1
  gfx_draw_bar(r, COLOR_WHITE);
  qr_scale = 2;
  text_offset = 9;
  bold.fg_color = COLOR_BLACK;
  bold.bg_color = COLOR_WHITE;
#else
  gfx_draw_bar(gfx_rect_shrink(r, 3), COLOR_WHITE);
  gfx_draw_bar(gfx_rect_shrink(r, 4), COLOR_BLACK);
#endif

  char dom[32];
  // format: {MODEL_IDENTIFIER}YYMMDD
  if (sectrue == flash_otp_read(FLASH_OTP_BLOCK_BATCH, 0, (uint8_t *)dom, 32) &&
      dom[31] == 0 && cstr_starts_with(dom, MODEL_IDENTIFIER)) {
    gfx_offset_t pos;

    pos = gfx_offset(DISPLAY_RESX / 2, DISPLAY_RESY / 2);
    gfx_draw_qrcode(pos, qr_scale, dom);

    pos = gfx_offset(DISPLAY_RESX / 2, DISPLAY_RESY - text_offset);
    gfx_draw_text(pos, dom + sizeof(MODEL_IDENTIFIER) - 1, -1, &bold,
                  GFX_ALIGN_CENTER);
  }

  display_refresh();
}

static void drivers_init(void) {
  display_init(DISPLAY_RESET_CONTENT);

#ifdef USE_STORAGE_HWKEY
  secure_aes_init();
#endif
#ifdef USE_HASH_PROCESSOR
  hash_processor_init();
#endif
#ifdef USE_SD_CARD
  sdcard_init();
#endif
#ifdef USE_BUTTON
  button_init();
#endif
#ifdef USE_TOUCH
  touch_init();
#endif
#ifdef USE_SBU
  sbu_init();
#endif
#ifdef USE_HAPTIC
  haptic_init();
#endif
#ifdef USE_RGB_LED
  rgb_led_init();
#endif
}

#define BACKLIGHT_NORMAL 150

int main(void) {
  system_init(&rsod_panic_handler);

  drivers_init();
  usb_init_all();

  // Draw welcome screen
  draw_welcome_screen();
  display_fade(0, BACKLIGHT_NORMAL, 1000);

  // Initialize command line interface
  cli_init(&g_cli, console_read, console_write, NULL);

  extern cli_command_t _prodtest_cli_cmd_section_start;
  extern cli_command_t _prodtest_cli_cmd_section_end;

  cli_set_commands(
      &g_cli, &_prodtest_cli_cmd_section_start,
      &_prodtest_cli_cmd_section_end - &_prodtest_cli_cmd_section_start);

#ifdef USE_OPTIGA
  optiga_init();
  optiga_open_application();
  pair_optiga(&g_cli);
#endif

  cli_run_loop(&g_cli);

  return 0;
}
