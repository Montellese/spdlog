//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include "spdlog/formatter.h"
#include "spdlog/details/log_msg.h"
#include "spdlog/details/os.h"
#include "spdlog/fmt/fmt.h"

#include <chrono>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <array>

namespace spdlog
{
namespace details
{
class flag_formatter
{
public:
    flag_formatter(unsigned long length = 0)
        : _length(length)
    {}
    virtual ~flag_formatter()
    {}
    virtual void format(details::log_msg& msg, const std::tm& tm_time) = 0;
protected:
    unsigned long _length;
    fmt::MemoryWriter& pad_space_left(fmt::MemoryWriter& w, const std::string& str)
    {
      if (str.size() < _length)
        w << std::string(_length - str.size(), ' ');
      w << str;
      return w;
    }
    fmt::MemoryWriter& pad_space_right(fmt::MemoryWriter& w, const std::string& str)
    {
        w << str;
        if (str.size() < _length)
            w << std::string(_length - str.size(), ' ');
        return w;
    }
    fmt::MemoryWriter& pad_space_right(fmt::MemoryWriter& w, size_t val)
    {
      const auto padded_format = fmt::format("{{: <{}}}", _length);
      w << fmt::format(padded_format, val);
      return w;
    }
};

///////////////////////////////////////////////////////////////////////
// name & level pattern appenders
///////////////////////////////////////////////////////////////////////
namespace
{
class name_formatter:public flag_formatter
{
public:
    name_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm&) override
    {
        pad_space_left(msg.formatted, *msg.logger_name);
    }
};
}

// log level appender
class level_formatter:public flag_formatter
{
public:
    level_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm&) override
    {
        pad_space_left(msg.formatted, level::to_str(msg.level));
    }
};

// short log level appender
class short_level_formatter:public flag_formatter
{
public:
    short_level_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm&) override
    {
        pad_space_left(msg.formatted, level::to_short_str(msg.level));
    }
};

///////////////////////////////////////////////////////////////////////
// Date time pattern appenders
///////////////////////////////////////////////////////////////////////

static const char* ampm(const tm& t)
{
    return t.tm_hour >= 12 ? "PM" : "AM";
}

static int to12h(const tm& t)
{
    return t.tm_hour > 12 ? t.tm_hour - 12 : t.tm_hour;
}

//Abbreviated weekday name
using days_array = std::array<std::string, 7>;
static const days_array& days()
{
    static const days_array arr{ { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" } };
    return arr;
}
class a_formatter:public flag_formatter
{
public:
    a_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        pad_space_right(msg.formatted, days()[tm_time.tm_wday]);
    }
};
// message counter formatter
class i_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm&) override
    {
        msg.formatted << '#' << msg.msg_id;
    }
};
//Full weekday name
static const days_array& full_days()
{
    static const days_array arr{ { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" } };
    return arr;
}
class A_formatter SPDLOG_FINAL :public flag_formatter
{
public:
    A_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        pad_space_right(msg.formatted, full_days()[tm_time.tm_wday]);
    }
};

//Abbreviated month
using months_array = std::array<std::string, 12>;
static const months_array& months()
{
    static const months_array arr{ { "Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec" } };
    return arr;
}
class b_formatter:public flag_formatter
{
public:
    b_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        pad_space_right(msg.formatted, months()[tm_time.tm_mon]);
    }
};

//Full month name
static const months_array& full_months()
{
    static const months_array arr{ { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" } };
    return arr;
}
class B_formatter SPDLOG_FINAL :public flag_formatter
{
public:
    B_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        pad_space_right(msg.formatted, full_months()[tm_time.tm_mon]);
    }
};


//write 2 ints seperated by sep with padding of 2
static fmt::MemoryWriter& pad_n_join(fmt::MemoryWriter& w, int v1, int v2, char sep)
{
    w << fmt::pad(v1, 2, '0') << sep << fmt::pad(v2, 2, '0');
    return w;
}

//write 3 ints seperated by sep with padding of 2
static fmt::MemoryWriter& pad_n_join(fmt::MemoryWriter& w, int v1, int v2, int v3, char sep)
{
    w << fmt::pad(v1, 2, '0') << sep << fmt::pad(v2, 2, '0') << sep << fmt::pad(v3, 2, '0');
    return w;
}


//Date and time representation (Thu Aug 23 15:35:46 2014)
class c_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << days()[tm_time.tm_wday] << ' ' << months()[tm_time.tm_mon] << ' ' << tm_time.tm_mday << ' ';
        pad_n_join(msg.formatted, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ':') << ' ' << tm_time.tm_year + 1900;
    }
};


// year - 2 digit
class C_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << fmt::pad(tm_time.tm_year % 100, 2, '0');
    }
};



// Short MM/DD/YY date, equivalent to %m/%d/%y 08/23/01
class D_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        pad_n_join(msg.formatted, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_year % 100, '/');
    }
};


// year - 4 digit
class Y_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << tm_time.tm_year + 1900;
    }
};

// month 1-12
class m_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << fmt::pad(tm_time.tm_mon + 1, 2, '0');
    }
};

// day of month 1-31
class d_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << fmt::pad(tm_time.tm_mday, 2, '0');
    }
};

// hours in 24 format  0-23
class H_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << fmt::pad(tm_time.tm_hour, 2, '0');
    }
};

// hours in 12 format  1-12
class I_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << fmt::pad(to12h(tm_time), 2, '0');
    }
};

// minutes 0-59
class M_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << fmt::pad(tm_time.tm_min, 2, '0');
    }
};

// seconds 0-59
class S_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << fmt::pad(tm_time.tm_sec, 2, '0');
    }
};

// milliseconds
class e_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm&) override
    {
        auto duration = msg.time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
        msg.formatted << fmt::pad(static_cast<int>(millis), 3, '0');
    }
};

// microseconds
class f_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm&) override
    {
        auto duration = msg.time.time_since_epoch();
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;
        msg.formatted << fmt::pad(static_cast<int>(micros), 6, '0');
    }
};

// nanoseconds
class F_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm&) override
    {
        auto duration = msg.time.time_since_epoch();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % 1000000000;
        msg.formatted << fmt::pad(static_cast<int>(ns), 9, '0');
    }
};

// AM/PM
class p_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        msg.formatted << ampm(tm_time);
    }
};


// 12 hour clock 02:55:02 pm
class r_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        pad_n_join(msg.formatted, to12h(tm_time), tm_time.tm_min, tm_time.tm_sec, ':') << ' ' << ampm(tm_time);
    }
};

// 24-hour HH:MM time, equivalent to %H:%M
class R_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        pad_n_join(msg.formatted, tm_time.tm_hour, tm_time.tm_min, ':');
    }
};

// ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S
class T_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
        pad_n_join(msg.formatted, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, ':');
    }
};

// ISO 8601 offset from UTC in timezone (+-HH:MM)
class z_formatter SPDLOG_FINAL:public flag_formatter
{
public:
    const std::chrono::seconds cache_refresh = std::chrono::seconds(5);

    z_formatter():_last_update(std::chrono::seconds(0)), _offset_minutes(0)
    {}
    z_formatter(const z_formatter&) = delete;
    z_formatter& operator=(const z_formatter&) = delete;

    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
#ifdef _WIN32
        int total_minutes = get_cached_offset(msg, tm_time);
#else
        // No need to chache under gcc,
        // it is very fast (already stored in tm.tm_gmtoff)
        int total_minutes = os::utc_minutes_offset(tm_time);
#endif
        bool is_negative = total_minutes < 0;
        char sign;
        if (is_negative)
        {
            total_minutes = -total_minutes;
            sign = '-';
        }
        else
        {
            sign = '+';
        }

        int h = total_minutes / 60;
        int m = total_minutes % 60;
        msg.formatted << sign;
        pad_n_join(msg.formatted, h, m, ':');
    }
private:
    log_clock::time_point _last_update;
    int _offset_minutes;
    std::mutex _mutex;

    int get_cached_offset(const log_msg& msg, const std::tm& tm_time)
    {
        using namespace std::chrono;
        std::lock_guard<std::mutex> l(_mutex);
        if (msg.time - _last_update >= cache_refresh)
        {
            _offset_minutes = os::utc_minutes_offset(tm_time);
            _last_update = msg.time;
        }
        return _offset_minutes;
    }
};



// Thread id
class t_formatter SPDLOG_FINAL:public flag_formatter
{
public:
    t_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm&) override
    {
        pad_space_right(msg.formatted, msg.thread_id);
    }
};

// Current pid
class pid_formatter SPDLOG_FINAL:public flag_formatter
{
    pid_formatter(unsigned long length = 0)
        : flag_formatter(length)
    {}
    void format(details::log_msg& msg, const std::tm&) override
    {
        pad_space_right(msg.formatted, details::os::pid());
    }
};


class v_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm&) override
    {
        msg.formatted << fmt::StringRef(msg.raw.data(), msg.raw.size());
    }
};

class ch_formatter SPDLOG_FINAL:public flag_formatter
{
public:
    explicit ch_formatter(char ch): _ch(ch)
    {}
    void format(details::log_msg& msg, const std::tm&) override
    {
        msg.formatted << _ch;
    }
private:
    char _ch;
};


//aggregate user chars to display as is
class aggregate_formatter SPDLOG_FINAL:public flag_formatter
{
public:
    aggregate_formatter()
    {}
    void add_ch(char ch)
    {
        _str += ch;
    }
    void format(details::log_msg& msg, const std::tm&) override
    {
        msg.formatted << _str;
    }
private:
    std::string _str;
};

// Full info formatter
// pattern: [%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v
class full_formatter SPDLOG_FINAL:public flag_formatter
{
    void format(details::log_msg& msg, const std::tm& tm_time) override
    {
#ifndef SPDLOG_NO_DATETIME
        auto duration = msg.time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

        /* Slower version(while still very fast - about 3.2 million lines/sec under 10 threads),
        msg.formatted.write("[{:d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}] [{}] [{}] {} ",
        tm_time.tm_year + 1900,
        tm_time.tm_mon + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec,
        static_cast<int>(millis),
        msg.logger_name,
        level::to_str(msg.level),
        msg.raw.str());*/


        // Faster (albeit uglier) way to format the line (5.6 million lines/sec under 10 threads)
        msg.formatted << '[' << static_cast<unsigned int>(tm_time.tm_year + 1900) << '-'
                      << fmt::pad(static_cast<unsigned int>(tm_time.tm_mon + 1), 2, '0') << '-'
                      << fmt::pad(static_cast<unsigned int>(tm_time.tm_mday), 2, '0') << ' '
                      << fmt::pad(static_cast<unsigned int>(tm_time.tm_hour), 2, '0') << ':'
                      << fmt::pad(static_cast<unsigned int>(tm_time.tm_min), 2, '0') << ':'
                      << fmt::pad(static_cast<unsigned int>(tm_time.tm_sec), 2, '0') << '.'
                      << fmt::pad(static_cast<unsigned int>(millis), 3, '0') << "] ";

        //no datetime needed
#else
        (void)tm_time;
#endif

#ifndef SPDLOG_NO_NAME
        msg.formatted << '[' << *msg.logger_name << "] ";
#endif

        msg.formatted << '[' << level::to_str(msg.level) << "] ";
        msg.formatted << fmt::StringRef(msg.raw.data(), msg.raw.size());
    }
};



}
}
///////////////////////////////////////////////////////////////////////////////
// pattern_formatter inline impl
///////////////////////////////////////////////////////////////////////////////
inline spdlog::pattern_formatter::pattern_formatter(const std::string& pattern, pattern_time_type pattern_time)
    : _pattern_time(pattern_time)
{
    compile_pattern(pattern);
}

inline void spdlog::pattern_formatter::compile_pattern(const std::string& pattern)
{
    auto end = pattern.end();
    std::unique_ptr<details::aggregate_formatter> user_chars;
    std::string length_chars;
    for (auto it = pattern.begin(); it != end; ++it)
    {
        if (*it == '%')
        {
            if (user_chars) //append user chars found so far
                _formatters.push_back(std::move(user_chars));

            while (++it != end)
            {
                if (handle_flag(*it, length_chars))
                    break;
            }

            if (it == end)
                break;
        }
        else // chars not following the % sign should be displayed as is
        {
            if (!user_chars)
                user_chars = std::unique_ptr<details::aggregate_formatter>(new details::aggregate_formatter());
            user_chars->add_ch(*it);
        }
    }
    if (user_chars) //append raw chars found so far
    {
        _formatters.push_back(std::move(user_chars));
    }

}
inline bool spdlog::pattern_formatter::handle_flag(char flag, std::string& length_chars)
{
    // handle padding
    if (flag >= '0' && flag <= '9')
    {
        length_chars += flag;
        return false;
    }

    unsigned long length = 0;
    if (!length_chars.empty())
    {
      length = strtoul(length_chars.c_str(), nullptr, 10);
    }

    switch (flag)
    {
    // logger name
    case 'n':
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::name_formatter(length)));
        break;

    case 'l':
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::level_formatter(length)));
        break;

    case 'L':
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::short_level_formatter(length)));
        break;

    case('t'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::t_formatter(length)));
        break;

    case('v'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::v_formatter()));
        break;

    case('a'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::a_formatter(length)));
        break;

    case('A'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::A_formatter(length)));
        break;

    case('b'):
    case('h'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::b_formatter(length)));
        break;

    case('B'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::B_formatter(length)));
        break;
    case('c'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::c_formatter()));
        break;

    case('C'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::C_formatter()));
        break;

    case('Y'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::Y_formatter()));
        break;

    case('D'):
    case('x'):

        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::D_formatter()));
        break;

    case('m'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::m_formatter()));
        break;

    case('d'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::d_formatter()));
        break;

    case('H'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::H_formatter()));
        break;

    case('I'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::I_formatter()));
        break;

    case('M'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::M_formatter()));
        break;

    case('S'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::S_formatter()));
        break;

    case('e'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::e_formatter()));
        break;

    case('f'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::f_formatter()));
        break;
    case('F'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::F_formatter()));
        break;

    case('p'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::p_formatter()));
        break;

    case('r'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::r_formatter()));
        break;

    case('R'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::R_formatter()));
        break;

    case('T'):
    case('X'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::T_formatter()));
        break;

    case('z'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::z_formatter()));
        break;

    case ('+'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::full_formatter()));
        break;

    case ('P'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::pid_formatter(length)));
        break;

#if defined(SPDLOG_ENABLE_MESSAGE_COUNTER)
    case ('i'):
        _formatters.push_back(std::unique_ptr<details::flag_formatter>(new details::i_formatter()));
        break;
#endif

    default: //Unkown flag appears as is
        auto chars = std::unique_ptr<details::aggregate_formatter>(new details::aggregate_formatter());
        chars->add_ch('%');
        for (auto it = length_chars.begin(); it != length_chars.end(); ++it)
          chars->add_ch(*it);
        chars->add_ch(flag);
        _formatters.push_back(std::move(chars));
        break;
    }

    length_chars.clear();

    return true;
}

inline std::tm spdlog::pattern_formatter::get_time(details::log_msg& msg)
{
    if (_pattern_time == pattern_time_type::local)
        return details::os::localtime(log_clock::to_time_t(msg.time));
    else
        return details::os::gmtime(log_clock::to_time_t(msg.time));
}

inline void spdlog::pattern_formatter::format(details::log_msg& msg)
{

#ifndef SPDLOG_NO_DATETIME
    auto tm_time = get_time(msg);
#else
    std::tm tm_time;
#endif
    for (auto &f : _formatters)
    {
        f->format(msg, tm_time);
    }
    //write eol
    msg.formatted.write(details::os::eol, details::os::eol_size);
}
