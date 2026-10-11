#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <cstdint> // include this header for uint64_t
#include <vector>
#include <string.h>
#include <cstring>
#include <string>
#include <sstream>
#include <iterator>
#include <ctime>
#include <locale>
#include <iomanip>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem/operations.hpp>
#include <time.h>

#ifndef FPANEL_STATISTICS_LINUX_HELPERS_H
#define FPANEL_STATISTICS_LINUX_HELPERS_H

class Helpers
{
public:
    bool isDir(const std::string &dir) {
        struct stat fileInfo;
        stat(dir.c_str(), &fileInfo);
        if (S_ISDIR(fileInfo.st_mode)) {
            return true;
        } else {
            return false;
        }
    }

    time_t fromDateToTimestamp(std::string &date) {
        try {
            const boost::posix_time::ptime time = boost::posix_time::from_iso_extended_string(date);
            return to_time_t(time);
        }
        catch (...) {
            return 0;
        }
    }

    unsigned long long getFolderSize(const boost::filesystem::path &directory) {
        if (!boost::filesystem::exists(directory))
            return 0;

        if (boost::filesystem::is_directory(directory)) {
            unsigned long long int ret_size = 0;
            boost::filesystem::directory_iterator m_dir_itr(directory);

            for (m_dir_itr = boost::filesystem::begin(m_dir_itr);
                 m_dir_itr != boost::filesystem::end(m_dir_itr); ++m_dir_itr) {
                boost::filesystem::directory_entry m_dir_entry = *m_dir_itr;
                if (boost::filesystem::is_regular_file(m_dir_entry.path())) {
                    ret_size += boost::filesystem::file_size(m_dir_entry.path());
                } else if (boost::filesystem::is_directory(m_dir_entry.path())) {
                    ret_size += getFolderSize(m_dir_entry.path());
                }
            }
            return ret_size;
        } else if (boost::filesystem::is_regular_file(directory)) {
            return boost::filesystem::file_size(directory);
        }

        return 0;
    }

    bool isContains(std::string &search, std::string &needle) {
        if (search.find(needle) != std::string::npos) {
            return true;
        }
        return false;
    }

    bool is_number(const std::string &s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
    }

    void Split(std::vector<std::string> &tokens, std::string str, const std::string &delimiter) {
        boost::split(tokens, str, boost::is_any_of(delimiter.c_str()));
    }

    std::string &ltrim(std::string &str, const std::string &chars = "\t\n\v\f\r ") {
        str.erase(0, str.find_first_not_of(chars));
        return str;
    }

    std::string &rtrim(std::string &str, const std::string &chars = "\t\n\v\f\r ") {
        str.erase(str.find_last_not_of(chars) + 1);
        return str;
    }

    std::string &trim(std::string &str, const std::string &chars = "\t\n\v\f\r ") {
        return ltrim(rtrim(str, chars), chars);
    }

    std::string CmdOutput(const std::string &cmd) {
        std::string data;
        FILE *stream;
        const int max_buffer = 8192;
        char buffer[max_buffer];

        stream = popen(cmd.c_str(), "r");
        if (stream) {
            while (!feof(stream)) {
                if (fgets(buffer, max_buffer, stream) != NULL) {
                    data.append(buffer);
                }
            }
            pclose(stream);
        }
        return trim(data);
    }

    std::vector<std::string> strSplit(std::string strToSplit, char delimeter)
    {
        std::stringstream ss(strToSplit);
        std::string item;
        std::vector<std::string> splittedStrings;
        while (std::getline(ss, item, delimeter)) {
            splittedStrings.push_back(std::string(item));
        }
        return splittedStrings;
    }

    long getTimestamp(std::string datetime)
    {
        char delimeter = '.';
        std::vector<std::string> datetime_arr = strSplit(datetime, delimeter);
        struct tm tm;
        time_t ts = 0;
        memset(&tm, 0, sizeof(tm));

        strptime(std::string(datetime_arr[0]).c_str(), "%d/%b/%Y:%H:%M:%S", &tm);
        ts = mktime(&tm);
        std::string ts_str = std::to_string(ts);
        return std::atol(ts_str.c_str());
    }

    bool hasEnding(std::string const &fullString, std::string const &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

    bool Replace(std::string &str, std::string old_str, std::string new_str) {
        boost::replace_all(str, old_str, new_str);
        return true;
    }

    // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
    std::string currentDateTime() {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
        return buf;
    }

    void removeSubStr(std::string &s, std::string &p) {
        std::string::size_type n = p.length();
        for (std::string::size_type i = s.find(p);
             i != std::string::npos;
             i = s.find(p))
            s.erase(i, n);
    }
};

#endif //FPANEL_STATISTICS_LINUX_HELPERS_H
