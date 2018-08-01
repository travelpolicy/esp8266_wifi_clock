#include <functional>

#include "fonts.h"

#define NUM_MAX 4

typedef const wchar_t* WSTR;

const WSTR weekdays[] = {
    L"Понедельник", 
    L"Вторник",
    L"Среда",
    L"Четверг",
    L"Пятница",
    L"Суббота",
    L"Воскресенье"
};

const WSTR monthes[] = {
    L"Января",
    L"Февраля",
    L"Марта",
    L"Апреля",
    L"Мая",
    L"Июня",
    L"Июля",
    L"Августа",
    L"Сентября",
    L"Октября",
    L"Ноября",
    L"Декабря"
};

const unsigned char midNumbers[][8] = {
  { 0x38, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38 }, // 0
  { 0x10, 0x30, 0x50, 0x10, 0x10, 0x10, 0x10, 0x7c }, // 1
  { 0x38, 0x44, 0x04, 0x08, 0x10, 0x20, 0x40, 0x7c }, // 2
  { 0x38, 0x44, 0x04, 0x18, 0x04, 0x04, 0x44, 0x38 }, // 3
  { 0x0c, 0x14, 0x24, 0x44, 0x7c, 0x04, 0x04, 0x04 }, // 4
  { 0x7c, 0x40, 0x40, 0x78, 0x04, 0x04, 0x44, 0x38 }, // 5
  { 0x3c, 0x40, 0x40, 0x78, 0x44, 0x44, 0x44, 0x38 }, // 6
  { 0x7c, 0x44, 0x04, 0x08, 0x10, 0x20, 0x20, 0x20 }, // 7
  { 0x38, 0x44, 0x44, 0x38, 0x44, 0x44, 0x44, 0x38 }, // 8
  { 0x38, 0x44, 0x44, 0x44, 0x3c, 0x04, 0x04, 0x38 }, // 9
};

const unsigned char smallNumbers[][8] = {
  { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xa0, 0xa0, 0xe0 }, // 0
  { 0x00, 0x00, 0x00, 0x40, 0xc0, 0x40, 0x40, 0xe0 }, // 1
  { 0x00, 0x00, 0x00, 0xe0, 0x20, 0xe0, 0x80, 0xe0 }, // 2
  { 0x00, 0x00, 0x00, 0xe0, 0x20, 0xe0, 0x20, 0xe0 }, // 3
  { 0x00, 0x00, 0x00, 0xa0, 0xa0, 0xe0, 0x20, 0x20 }, // 4
  { 0x00, 0x00, 0x00, 0xe0, 0x80, 0xe0, 0x20, 0xe0 }, // 5
  { 0x00, 0x00, 0x00, 0xe0, 0x80, 0xe0, 0xa0, 0xe0 }, // 6
  { 0x00, 0x00, 0x00, 0xe0, 0x20, 0x20, 0x40, 0x40 }, // 7
  { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xe0, 0xa0, 0xe0 }, // 8
  { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xe0, 0x20, 0xe0 }, // 9
};

class Figure {
public:
    virtual void pixels(std::function<void(int, int)> acceptor) const = 0;
};

class Rectangle : public Figure {
public:
    const int x;
    const int y;
    const int w;
    const int h;

    Rectangle() : x(0), y(0), w(0), h(0) {}
    Rectangle(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}

    virtual void pixels(std::function<void(int, int)> acceptor) const {
        for (int xx = x; xx < x + w; ++xx) {
            for (int yy = y; yy < y + h; ++yy) {
                acceptor(xx, yy);
            }
        }
    }
};

class Bitmask : public Figure {
    typedef const uint8_t BitsContent[8];
public:
    const BitsContent& bits;
    Bitmask(const BitsContent& _bits) : bits(_bits) {
    }

    virtual void pixels(std::function<void(int, int)> acceptor) const {
        for (int yy = 0; yy < sizeof(bits); ++yy) {
            for (int i = 0; i < 8; ++i) {
                if ((bits[yy] >> i) & 1) {
                    acceptor(i, yy);
                }
            }
        }
    }
};

const uint8_t leapYearMonth[] = {
    31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
const uint8_t regularYearMonth[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};


class LcdScreen {

private:
    uint8_t screen[NUM_MAX * 8];

public:
    static const int secInUs = 1000000;

    LcdScreen() {
        memset(screen, 0, sizeof(screen));
    }

    bool fits(int x, int y) {
        return (x >= 0) && (y >= 0) && (x < width()) && (y < height());
    }

    int idx(int x, int y) {
        int res = x / 8 * 8 + y;
        return res;
    }

    bool get(int x, int y) {
        if (!fits(x, y)) {
            return false; // Outside world is black
        }
        return (screen[idx(x, y)] >> (x % 8)) & 1;
    }

    void set(int x, int y, bool clr) {
        if (fits(x, y)) {
            screen[idx(x, y)] &= ~(1 << (x % 8));
            screen[idx(x, y)] |= (clr ? 1 : 0) << (x % 8);
        }
    }

    void invert(int x, int y) {
        set(x, y, !get(x, y));
    }

    void invert(int _x, int _y, const Figure& fg) {
        fg.pixels([=](int x, int y) {
            this->invert(_x + x, _y + y);
        });
    }

    void set(int _x, int _y, const Figure& fg, bool clr) {
        fg.pixels([=](int x, int y) {
            this->set(_x + x, _y + y, clr);
        });
    }

    void clear() {
        memset(screen, 0, sizeof(screen));
    }

    uint8_t line8(int l) const {
        return screen[l];
    }

    int height() const {
        return 8;
    }

    int width() const {
        return NUM_MAX*8;
    }

    /**
     * This could be hour, min or sec
     */
    class TimeComponent {
        int val = 0;
        char asStr[4];
    public:
        TimeComponent(int _val) : val(_val) {
            memset(asStr, 0, sizeof(asStr));
            snprintf(asStr, sizeof(asStr), "%02d", val);
        }

        uint8_t charAt(int ind) {
            return asStr[ind];
        }

        const char* c_str() {
            return asStr;
        }
    };

    const uint8_t* symbolPtrOrNull(wchar_t symbol) {
        int fontItem = fontUA[0];
        int sym1251 = symbol & 0xffff;
        if (sym1251 >= 0x410 && sym1251 < 0x450) {
            sym1251 = sym1251 - 0x410 + 0xa0;
        } else {
            sym1251 = sym1251 - 0x20;
        }
        if (sym1251 >= 0 && sym1251 < (sizeof(fontUA) - 1)/fontItem) {
            return fontUA + 1 + sym1251*fontItem;
        }
        return NULL;
    }

    int getStrWidth(const WSTR* strs, int strsCnt) {
        int w = 0;
        for (;strsCnt > 0; --strsCnt, ++strs) {
            w += getStrWidth(*(strs)) + 1;
        }
        return w;
    }

    int getStrWidth(WSTR str) {
        int res = 0;
        for (int i=0; str[i] != 0; ++i) {
            const uint8_t* symbol = symbolPtrOrNull(str[i]);
            if (symbol != NULL) {
                if (res > 0) {
                    res += 1;
                }
                res += symbol[0];
            }
        }
        return res;
    }

    /**
     * Prints several strings at once
     */
    int printStr(int _x, int _y, const WSTR* strs, int strsCnt) {
        int ww = 0;
        for (;strsCnt > 0; --strsCnt, ++strs) {
            int w = printStr(_x, _y, *(strs));
            ww += w + 1;
            _x -= w + 1;
        }
        return ww;
    }

    /**
     * Prints string and returns printed string width
     */
    int printStr(int _x, int _y, WSTR str) {
        int w = 0;
        for (int i=0; str[i] != 0; ++i) {
            const uint8_t* symbol = symbolPtrOrNull(str[i]);
            if (symbol > fontUA && symbol < (fontUA + sizeof(fontUA))) {
                int symbolW = *symbol & 0xff;
                
                for (int x = 0; x < symbolW; ++x) {
                    uint8_t data = symbol[1 + x];
                    for (int y = 0; y < 8; ++y) {
                        set(_x - w - x, y + _y, (data >> y) & 1);
                    }
                }
                w += symbolW + 1;
            }
        }
        return w;        
    }

    struct RTC {
        uint16_t dow;
        uint16_t day;
        uint16_t month;
        uint16_t year; // 
        bool leapYear;
        uint16_t doy; // 0-based
    };

    static void epoc2rtc(uint32_t t, RTC &rtc) {
        const int cycle400year = 365*400 + (100 - 3);
        const int cycle100year = 365*100 + 25 - 1;
        const int cycle4year = 365*4 + 3;
        rtc.dow = (t + 3) % 7; // Day of week
        rtc.year = 1970 + 
            t / cycle400year * 400 + 
            t / cycle100year * 100 + 
            t / cycle4year * 4;
        rtc.leapYear = rtc.year % 400 == 0 || (rtc.year % 100 != 0 && rtc.year % 4 == 0);

        const uint8_t* dm = rtc.leapYear ? leapYearMonth : regularYearMonth;

        rtc.doy = (((t + 719536) % cycle400year) % cycle100year) % cycle4year;
        if (rtc.doy > 366) {
            rtc.doy -= 366;
        }
        if (rtc.doy > 365) {
            rtc.doy -= 365;
        }
        if (rtc.doy > 365) {
            rtc.doy -= 365;
        } 
        rtc.month = 0;
        int d = rtc.doy;
        for (;d > 0;) {
            d -= dm[rtc.month];
            rtc.month++;
        }
        rtc.day = d;
    }

    /**
     * micros is current time in microseconds
     */
    void showTime(uint32_t daysSince1970, uint32_t millisSince1200) {
        printf("%d\n", daysSince1970);
        if (millisSince1200 / 1000 % 30 < 10) {
            RTC rtc = {0};
            epoc2rtc(daysSince1970, rtc);
            wchar_t yearStr[10] = { 0 };
            swprintf(yearStr, sizeof(yearStr)/sizeof(yearStr[0]), L"%d", rtc.year);
            wchar_t dayStr[10] = { 0 };
            swprintf(dayStr, sizeof(dayStr)/sizeof(dayStr[0]), L"%d", rtc.day+1);
            const WSTR ss[] = {
                L"  ", 
                weekdays[rtc.dow],
                L", ",
                dayStr,
                L" ",
                monthes[rtc.month],
                L" ",
                yearStr,
                L" года ...  "
            };
            printStr((micros() / 1000 / 50) % getStrWidth(ss, sizeof(ss)/sizeof(ss[0])), 0, ss, sizeof(ss)/sizeof(ss[0]));
            return;
        }
        
        int seconds = millisSince1200 / 1000;
        TimeComponent hours((seconds % 86400L) / 3600);
        TimeComponent mins((seconds % 3600) / 60);

        TimeComponent secs(seconds % 60);
        TimeComponent nextSecs((seconds + 1) % 60);
        TimeComponent nextNextSecs((seconds + 2) % 60);

        clear();
        // Hours
        for(int n = 0; n < 2; n++) {
            set(width() - 1 - (n+1)*6, 0, 
                Bitmask(midNumbers[hours.charAt(n) - '0']), true);
        }
        // Mins
        for(int n = 0; n < 2; n++) {
            set(18 - (n+1)*6, 0, 
                Bitmask(midNumbers[mins.charAt(n) - '0']), true);
        }

        int movingTimeUs = 1000 / 4; // Period of time to do seconds moving transition
        int smallFontHeight = 6;
        for(int n = 1; n >= 0; n--) {
            int y;
            int ms  = (millisSince1200 % 1000);
            if (ms >= (1000 - movingTimeUs)) {
                y = (smallFontHeight - (1000 - ms) * smallFontHeight / movingTimeUs);
            } else {
                y = 0;
            }

            invert(n*4 - 5, y, Bitmask(smallNumbers[secs.charAt(1-n) - '0']));
            invert(n*4 - 5, y-smallFontHeight, Bitmask(smallNumbers[nextSecs.charAt(1-n) - '0']));
            invert(n*4 - 5, y-smallFontHeight*2, Bitmask(smallNumbers[nextNextSecs.charAt(1-n) - '0']));

            // printf("%s %s %s -> %d %ul\n", nextNextSecs.c_str(), secs.c_str(), nextSecs.c_str(), y, us); 
        }
    }
};

//======================================================================================================

class MAX72xx {
    // Opcodes for the MAX7221 and MAX7219
    // All OP_DIGITn are offsets from OP_DIGIT0
    #define	OP_NOOP         0 ///< MAX72xx opcode for NO OP
    #define OP_DIGIT0       1 ///< MAX72xx opcode for DIGIT0
    #define OP_DIGIT1       2 ///< MAX72xx opcode for DIGIT1
    #define OP_DIGIT2       3 ///< MAX72xx opcode for DIGIT2
    #define OP_DIGIT3       4 ///< MAX72xx opcode for DIGIT3
    #define OP_DIGIT4       5 ///< MAX72xx opcode for DIGIT4
    #define OP_DIGIT5       6 ///< MAX72xx opcode for DIGIT5
    #define OP_DIGIT6       7 ///< MAX72xx opcode for DIGIT6
    #define OP_DIGIT7       8 ///< MAX72xx opcode for DIGIT7
    #define OP_DECODEMODE   9 ///< MAX72xx opcode for DECODE MODE
    #define OP_INTENSITY   10 ///< MAX72xx opcode for SET INTENSITY
    #define OP_SCANLIMIT   11 ///< MAX72xx opcode for SCAN LIMIT
    #define OP_SHUTDOWN    12 ///< MAX72xx opcode for SHUT DOWN
    #define OP_DISPLAYTEST 15 ///< MAX72xx opcode for DISPLAY TEST

public:
    MAX72xx(const LcdScreen& _screen, 
            const int _CLK_PIN,
            const int _DATA_PIN,
            const int _CS_PIN) : 
            screen(_screen), 
            CLK_PIN(_CLK_PIN),
            DATA_PIN(_DATA_PIN),
            CS_PIN(_CS_PIN) {
    }

    void sendCmd(int addr, uint8_t cmd, uint8_t data) {
        digitalWrite(CS_PIN, LOW);
        for (int i = NUM_MAX - 1; i >= 0; i--) {
            shiftOut(DATA_PIN, CLK_PIN, MSBFIRST, i == addr ? cmd : 0);
            shiftOut(DATA_PIN, CLK_PIN, MSBFIRST, i == addr ? data : 0);
        }
        digitalWrite(CS_PIN, HIGH);
    }

    void sendCmdAll(uint8_t cmd, uint8_t data) {
        digitalWrite(CS_PIN, LOW);
        for (int i = NUM_MAX - 1; i >= 0; i--) {
            shiftOut(DATA_PIN, CLK_PIN, MSBFIRST, cmd);
            shiftOut(DATA_PIN, CLK_PIN, MSBFIRST, data);
        }
        digitalWrite(CS_PIN, HIGH);
    }

    void setup() {
        pinMode(CS_PIN, OUTPUT);
        pinMode(DATA_PIN, OUTPUT);
        pinMode(CLK_PIN, OUTPUT);

        digitalWrite(CS_PIN, HIGH);
        sendCmdAll(OP_DISPLAYTEST, 0);
        sendCmdAll(OP_SCANLIMIT, 7);
        sendCmdAll(OP_DECODEMODE, 0);
        sendCmdAll(OP_SHUTDOWN, 1);
        sendCmdAll(OP_INTENSITY, 0); // minimum brightness
    }

    void refreshAll() {
        for (int line = 0; line < 8; line++) {
            digitalWrite(CS_PIN, LOW);
            for (int chip = NUM_MAX - 1; chip >= 0; chip--) {
                shiftOut(DATA_PIN, CLK_PIN, MSBFIRST, OP_DIGIT0 + line);
                shiftOut(DATA_PIN, CLK_PIN, MSBFIRST, screen.line8(chip * 8 + line));
            }
            digitalWrite(CS_PIN, HIGH);
        }
        digitalWrite(CS_PIN, LOW);
    }

  private:
    const int CLK_PIN;
    const int DATA_PIN;
    const int CS_PIN;

    const LcdScreen& screen;
};

//======================================================================================================
