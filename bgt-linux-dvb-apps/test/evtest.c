/*
 * $Id: evtest.c,v 1.3 2005/08/15 20:43:52 js Exp $
 *
 *  Copyright (c) 1999-2000 Vojtech Pavlik
 *
 *  Event device test program
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

#include <linux/input.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

char *events[EV_MAX + 1] = { "Sync", "Key", "Relative", "Absolute", "Misc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
NULL, NULL, NULL, "LED", "Sound", NULL, "Repeat", "ForceFeedback", "Power", "ForceFeedbackStatus"};
char *syncs[2] = { "Sync", "Config"};
char *keys[KEY_MAX + 1] = {
  // 0x000
  "Reserved", "Esc", "1", "2", "3", "4", "5", "6",
  "7", "8", "9", "0", "Minus", "Equal", "Backspace", "Tab",
  // 0x010
  "Q", "W", "E", "R", "T", "Y", "U", "I",
  "O", "P", "LeftBrace", "RightBrace", "Enter", "LeftControl", "A", "S",
  // 0x020
  "D", "F", "G", "H", "J", "K", "L", "Semicolon",
  "Apostrophe", "Grave", "LeftShift", "BackSlash", "Z", "X", "C", "V",
  // 0x030
  "B", "N", "M", "Comma", "Dot", "Slash", "RightShift", "KPAsterisk",
  "LeftAlt", "Space", "CapsLock", "F1", "F2", "F3", "F4", "F5",
  // 0x040
  "F6", "F7", "F8", "F9", "F10", "NumLock", "ScrollLock", "KP7",
  "KP8", "KP9", "KPMinus", "KP4", "KP5", "KP6", "KPPlus", "KP1",
  // 0x050
  "KP2", "KP3", "KP0", "KPDot", NULL, "ZENKAKUHANKAKU", "102nd", "F11",
  "F12", "RO", "KATAKANA", "HIRAGANA", "HENKAN", "KATAKANAHIRAGANA", "MUHENKAN", "KPJPCOMMA",
  // 0x060
  "KPEnter", "RightCtrl", "KPSlash", "SysRq", "RightAlt", "LineFeed", "Home", "Up",
  "PageUp", "Left", "Right", "End", "Down", "PageDown", "Insert", "Delete",
  // 0x070
  "Macro", "Mute", "VolumeDown", "VolumeUp", "Power", "KPEqual", "KPPlusMinus", "Pause",
  NULL, "KPComma", "HANGUEL", "HANJA", "YEN", "LeftMeta", "RightMeta", "Compose",
  // 0x080
  "Stop", "Again", "Props", "Undo", "Front", "Copy", "Open", "Paste",
  "Find", "Cut", "Help", "Menu", "Calc", "Setup", "Sleep", "WakeUp",
  // 0x090
  "File", "SendFile", "DeleteFile", "X-fer", "Prog1", "Prog2", "WWW", "MSDOS",
  "Coffee", "Direction", "CycleWindows", "Mail", "Bookmarks", "Computer", "Back", "Forward",
  // 0x0A0
  "CloseCD", "EjectCD", "EjectCloseCD", "NextSong", "PlayPause", "PreviousSong", "StopCD", "Record",
  "Rewind", "Phone", "ISOKey", "Config", "HomePage", "Refresh", "Exit", "Move",
  // 0x0B0
  "Edit", "ScrollUp", "ScrollDown", "KPLeftParenthesis", "KPRightParenthesis", NULL, NULL, "F13",
  "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21",
  // 0x0C0
  "F22", "F23", "F24", NULL, NULL, NULL, NULL, NULL,
  "PlayCD", "PauseCD", "Prog3", "Prog4", NULL, "Suspend", "Close", "Play",
  // 0x0D0
  "FastForward", "BassBoost", "Print", "HP", "Camera", "Sound", "Question", "EMail",
  "Chat", "Search", "Connect", "Finance", "Sport", "Shop", "AltErase", "Cancel",
  // 0x0E0
  "BrightnessDown", "BrightnessUp", "Media", NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x0F0
  "Unknown", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x100
  "Btn0", "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6", "Btn7",
  "Btn8", "Btn9", NULL, NULL,  NULL, NULL, NULL, NULL,
  // 0x110
  "LeftBtn", "RightBtn", "MiddleBtn", "SideBtn", "ExtraBtn", "ForwardBtn", "BackBtn", "TaskBtn",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x120
  "Trigger", "ThumbBtn", "ThumbBtn2", "TopBtn", "TopBtn2", "PinkieBtn", "BaseBtn", "BaseBtn2",
  "BaseBtn3", "BaseBtn4", "BaseBtn5", "BaseBtn6", NULL, NULL, NULL, "BtnDead",
  // 0x130
  "BtnA", "BtnB", "BtnC", "BtnX", "BtnY", "BtnZ", "BtnTL", "BtnTR",
  "BtnTL2", "BtnTR2", "BtnSelect", "BtnStart", "BtnMode", "BtnThumbL", "BtnThumbR", NULL,
  // 0x140
  "ToolPen", "ToolRubber", "ToolBrush", "ToolPencil", "ToolAirbrush", "ToolFinger", "ToolMouse", "ToolLens",
  NULL, NULL, "Touch", "Stylus", "Stylus2", "ToolDoubleTap", "ToolTripleTap", NULL,
  // 0x150
  "GearDown", "GearUp", NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x160
  "Ok", "Select", "Goto", "Clear", "Power2", "Option", "Info", "Time",
  "Vendor", "Archive", "Program", "Channel", "Favorites", "EPG", "PVR", "MHP",
  // 0x170
  "Language", "Title", "Subtitle", "Angle", "Zoom", "Mode", "Keyboard", "Screen",
  "PC", "TV", "TV2", "VCR", "VCR2", "Sat", "Sat2", "CD",
  // 0x180
  "Tape", "Radio", "Tuner", "Player", "Text", "DVD", "Aux", "MP3",
  "Audio", "Video", "Directory", "List", "Memo", "Calendar", "Red", "Green",
  // 0x190
  "Yellow", "Blue", "ChannelUp", "ChannelDown", "First", "Last", "AB", "Next",
  "Restart", "Slow", "Shuffle", "Break", "Previous", "Digits", "Teen", "Twen",
  // 0x1A0
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x1B0
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x1C0
  "DelEOL", "DelEOS", "InsLine", "DelLine", NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x1D0
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x1E0
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x1F0
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

char *absval[5] = { "Value", "Min  ", "Max  ", "Fuzz ", "Flat " };
char *relatives[REL_MAX + 1] = {
  "X", "Y", "Z", NULL, NULL, NULL, "HWheel", "Dial",
  "Wheel", "Misc", NULL, NULL, NULL, NULL, NULL, NULL,
};
char *absolutes[ABS_MAX + 1] = {
  // 0x00
  "X", "Y", "Z", "Rx", "Ry", "Rz", "Throttle", "Rudder",
  "Wheel", "Gas", "Brake", NULL, NULL, NULL, NULL, NULL,
  // 0x10
  "Hat0X", "Hat0Y", "Hat1X", "Hat1Y", "Hat2X", "Hat2Y", "Hat3X", "Hat 3Y",
  "Pressure", "Distance", "XTilt", "YTilt", "ToolWidth", NULL, NULL, NULL,
  // 0x20
  "Volume", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "Misc", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  // 0x30
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};
char *leds[LED_MAX + 1] = { "NumLock", "CapsLock", "ScrollLock", "Compose", "Kana", "Sleep", "Suspend", "Mute" };
char *repeats[REP_MAX + 1] = { "Delay", "Period" };
char *sounds[SND_MAX + 1] = { "Bell", "Click" };

char **names[EV_MAX + 1] = { syncs, keys, relatives, absolutes, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
NULL, NULL, leds, sounds, NULL, repeats, NULL, NULL, NULL };

#ifndef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(long) * 8)
#endif
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

int main (int argc, char **argv)
{
	int fd, rd, i, j, k;
	struct input_event ev[64];
	int version;
	unsigned short id[4];
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	char name[256] = "Unknown";
	int _abs[5];

	if (argc < 2) {
		printf("Usage: evtest /dev/input/eventX\n");
		printf("Where X = input device number\n");
		exit(1);
	}

	if ((fd = open(argv[argc - 1], O_RDONLY)) < 0) {
		perror("evtest");
		exit(1);
	}

	if (ioctl(fd, EVIOCGVERSION, &version)) {
		perror("evtest: can't get version");
		exit(1);
	}

	printf("Input driver version is %d.%d.%d\n",
		version >> 16, (version >> 8) & 0xff, version & 0xff);

	ioctl(fd, EVIOCGID, id);
	printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n",
		id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("Input device name: \"%s\"\n", name);

	memset(bit, 0, sizeof(bit));
	ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
	printf("Supported events:\n");

	for (i = 0; i < EV_MAX; i++)
		if (test_bit(i, bit[0])) {
			printf("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
			ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
			for (j = 0; j < KEY_MAX; j++)
				if (test_bit(j, bit[i])) {
					printf("    Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
					if (i == EV_ABS) {
						ioctl(fd, EVIOCGABS(j), _abs);
						for (k = 0; k < 5; k++)
							if ((k < 3) || _abs[k])
								printf("      %s %6d\n", absval[k], _abs[k]);
					}
				}
		}


	printf("Testing ... (interrupt to exit)\n");

	while (1) {
		rd = read(fd, ev, sizeof(struct input_event) * 64);

		if (rd < (int) sizeof(struct input_event)) {
			printf("yyy\n");
			perror("\nevtest: error reading");
			exit (1);
		}

		for (i = 0; i < rd / (int) sizeof(struct input_event); i++)
			printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
				ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type,
				events[ev[i].type] ? events[ev[i].type] : "?",
				ev[i].code,
				names[ev[i].type] ? (names[ev[i].type][ev[i].code] ? names[ev[i].type][ev[i].code] : "?") : "?",
				ev[i].value);

	}
}
