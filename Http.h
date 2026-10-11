#pragma once
#ifndef FPANEL_STATISTICS_LINUX_HTTP_H
#define FPANEL_STATISTICS_LINUX_HTTP_H

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

class Http
{
private:
    Helpers helpers;
    int outgoing_byte_index = 6;
    unsigned long long http_traffic = 0;
    int time_index = 6;
    int outgoing_byte_index_default_vhost = 6;
    int time_index_default_vhost = 6;
    std::string str_dom_http_regex;
    std::string str_df_http_regex;
    std::string access_log_file_prefix = "access.log";
    time_t from_timestamp;
    std::string gz_prefix = ".gz";
    Document d;

public:
    void setFromTimestamp(time_t ft) {
        from_timestamp = ft;
    }

    void parseLogFormatForDefaultVhost(std::string &system_type, std::string &httpd_server) {
        if (httpd_server.compare("apache") != 0) {
            return;
        }
        /**
         * %h => client_ip
         * %l => ident of client_machine
         * %u => user id of client
         * %t => time
         * %r => method
         * %s => code
         * %b => size
         * %p => port
         * %O => Outgoing Byte
         * %I => Incomin Byte
         * %{Referer}i => referer
         * %{User-Agent}i => agent
         */

// Standard
// %h %l %u %t \"%r\" %>s %O \"%{Referer}i\" \"%{User-Agent}i\"
// CLIENT IDC UIDC TIME METHOD CODE BYTE REFERER AGENT
// LogIO
// "%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\" %I %O" combinedio
// CLIENT IDC UIDC TIME METHOD CODE BYTE REFERER AGENT
// CLIENT - - TIME METHOD URI HTTP_VER" CODE BYTE REFERER AGENT
// Read from the text file

        std::string conf_line;
        std::string conf_file;

        std::map<std::string, std::string> regex_array;
        regex_array["%h"] = "(\\S+)";
        regex_array["%l"] = "(\\S+)";
        regex_array["%u"] = "(\\S+)";
//        regex_array["%t"] = "\\[([\\w:/]+\\s[+\\-]\\d{4})\\]";
        regex_array["%t"] = R"(\[([\w:/]+ [+\-]\d{4})\])";
        regex_array["%r"] = R"(\"(\S+) (\S+) *(\S+)\")";
//    regex_array["%r"] = "\\\"(\\S+) (\\S+)\\s*(\\S+)\\\"";
        regex_array["%s"] = "(\\d{3})";
        regex_array["%>s"] = "(\\d{3})";
        regex_array["%b"] = "(\\S+)";
        regex_array["%{Referer}i"] = R"(\"(\S+)\")";
        regex_array["%{User-Agent}i"] = R"(\"([^\"]*)\")";
        regex_array["%I"] = "(\\S+)";
        regex_array["%p"] = "(\\S+)";
        regex_array["%O"] = "(\\d+)";
        regex_array["%^FB"] = "(\\S+)";
        regex_array["%S"] = "(\\S+)";

        // ^(\S+) (\S+) (\S+) \[([\w:/]+ [+\-]\d{4})\] \"(\S+) (\S+) *(\S+)\" (\d{3}) (\S+) \"(\S+)\" \"([^\"]*)\"$             // Regexp
        // ^%h    %l    %u    %t                        \"%r                 \" %s       %O   \"%{Referer}i\" \"%{User-Agent}i\"  // Log Config => Bosluga gore split et ve yukaridakine gore

        if (system_type.empty()) {
            std::cout << "Unsupported system. Please install Debian, Ubuntu, Centos or Windows." << std::endl;
            exit(1);
        }
        bool combinedio = false;

        if (system_type.compare("centos") == 0) {
            conf_file = "/etc/httpd/conf/httpd.conf";
            std::string active_modules = helpers.CmdOutput(
                    "httpd -M | grep logio_module | grep -v NameVirtualHost >/dev/null 2>&1; RETVAL=$?; echo $RETVAL");
            if (active_modules == "0")
                combinedio = true;
        }
        else {
            conf_file = "/etc/apache2/apache2.conf";
            std::string active_modules = helpers.CmdOutput(
                    "apache2ctl -M | grep logio_module | grep -v NameVirtualHost >/dev/null 2>&1; RETVAL=$?; echo $RETVAL");
            helpers.trim(active_modules);
            if (active_modules.compare("0") == 0)
                combinedio = true;
        }

        std::ifstream httpd_conf(conf_file);
        std::string combined_log_config;
        std::string combinedio_log_config;
        std::vector<std::string> conf_array;

        bool logio = false;
        if (combinedio) {
            while (getline(httpd_conf, conf_line)) {
                std::string line = helpers.trim(conf_line);
                if (line.rfind("LogFormat", 0) == 0) {
                    if (helpers.hasEnding(line, "combinedio")) {
                        helpers.Replace(line, "LogFormat", "");
                        helpers.Replace(line, "combinedio", "");
                        combined_log_config = helpers.trim(line);
                        helpers.Split(conf_array, combined_log_config, " ");
                        logio = true;
                        break;
                    }
                }
            }
        }

        if (!logio) {
//            std::cout << "combinedio isn't active" << std::endl;
            while (getline(httpd_conf, conf_line)) {
                std::string line = helpers.trim(conf_line);
                if (line.rfind("LogFormat", 0) == 0) {
                    if (helpers.hasEnding(line, "combined")) {
                        helpers.Replace(line, "LogFormat", "");
                        helpers.Replace(line, "combined", "");
                        combined_log_config = helpers.trim(line);
                        helpers.Split(conf_array, combined_log_config, " ");
                    }
                }
            }
        }

        std::string regex_string;
//    std::cout << "combined_log_config: " << combined_log_config << std::endl;
        for (std::size_t i = 0; i < conf_array.size(); ++i) {
            std::string item = conf_array[i];
            helpers.Replace(item, "\\\"", "");
            helpers.Replace(item, "\"", "");
            if (regex_string.empty()) {
                regex_string = regex_array[item];
            }
            else {
                regex_string += " " + regex_array[item];
            }
        }

        std::vector<std::string> regex_string_array;
        helpers.Split(regex_string_array, regex_string, " ");
//    std::cout << "regex_string: " << regex_string << std::endl;

        for (std::size_t i = 0; i < regex_string_array.size(); ++i) {
            if (regex_string_array[i].compare("(\\d+)") == 0) {
//            std::cout << "outgoing_byte_index_default_vhost: " << i -1  << std::endl;
                outgoing_byte_index_default_vhost = i - 1;
                break;
            }
            if (regex_string_array[i].compare("[+\\-]\\d{4})\\]") == 0) {
//            std::cout << "time_index_default_vhost: " << i -1  << std::endl;
                time_index_default_vhost = i - 1;
                break;
            }
        }
        httpd_conf.close();
        str_df_http_regex = regex_string;
    }

    void parseLogFormatForDomainApache(std::string &system_type, std::string &domain) {
        /**
         * %h => client_ip
         * %l => ident of client_machine
         * %u => user id of client
         * %t => time
         * %r => method
         * %s => code
         * %b => size
         * %p => port
         * %O => Outgoing Byte
         * %I => Incomin Byte
         * %{Referer}i => referer
         * %{User-Agent}i => agent
         */

// Standard
// %h %l %u %t \"%r\" %>s %O \"%{Referer}i\" \"%{User-Agent}i\"
// CLIENT IDC UIDC TIME METHOD CODE BYTE REFERER AGENT
// LogIO
// "%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\" %I %O" combinedio
// CLIENT IDC UIDC TIME METHOD CODE BYTE REFERER AGENT
// CLIENT - - TIME METHOD URI HTTP_VER" CODE BYTE REFERER AGENT
// Read from the text file

        std::string conf_line;
        std::string domains_conf_file = "/usr/local/fpanel/configs/domains/" + domain + "/httpd.include";

        std::map<std::string, std::string> regex_array;

        regex_array["%h"] = "(\\S+)";
        regex_array["%l"] = "(\\S+)";
        regex_array["%u"] = "(\\S+)";
//    regex_array["%t"] = "\\[([\\w:/]+\\s[+\\-]\\d{4})\\]";
        regex_array["%t"] = R"(\[([\w:/]+ [+\-]\d{4})\])";
        regex_array["%r"] = R"(\"(\S+) (\S+) *(\S+)\")";
//    regex_array["%r"] = "\\\"(\\S+) (\\S+)\\s*(\\S+)\\\"";
        regex_array["%s"] = "(\\d{3})";
        regex_array["%>s"] = "(\\d{3})";
        regex_array["%b"] = "(\\S+)";
        regex_array["%{Referer}i"] = R"(\"(\S+)\")";
        regex_array["%{User-Agent}i"] = R"(\"([^\"]*)\")";
        regex_array["%I"] = "(\\S+)";
        regex_array["%p"] = "(\\S+)";
        regex_array["%O"] = "(\\d+)";
        regex_array["%^FB"] = "(\\S+)";
        regex_array["%S"] = "(\\S+)";

        // ^(\S+) (\S+) (\S+) \[([\w:/]+ [+\-]\d{4})\] \"(\S+) (\S+) *(\S+)\" (\d{3}) (\S+) \"(\S+)\" \"([^\"]*)\"$             // Regexp
        // ^%h    %l    %u    %t                        \"%r                 \" %s       %O   \"%{Referer}i\" \"%{User-Agent}i\"  // Log Config => Bosluga gore split et ve yukaridakine gore

        if (system_type.empty()) {
            std::cout << "Unsupported system. Please install Debian, Centos, Ubuntu or Windows." << std::endl;
            exit(1);
        }
        bool combinedio = false;

        if (system_type.compare("centos") == 0) {
            std::string active_modules = helpers.CmdOutput(
                    "httpd -M | grep logio_module | grep -v NameVirtualHost >/dev/null 2>&1; RETVAL=$?; echo $RETVAL");
            if (active_modules == "0")
                combinedio = true;
        }
        else {
            std::string active_modules = helpers.CmdOutput(
                    "apache2ctl -M | grep logio_module | grep -v NameVirtualHost >/dev/null 2>&1; RETVAL=$?; echo $RETVAL");
            if (active_modules == "0")
                combinedio = true;
        }

        std::ifstream domain_conf(domains_conf_file);
        std::string combined_log_config;
        std::string combinedio_log_config;
        std::vector<std::string> conf_array;

        bool logio = false;
        if (combinedio) {
            while (getline(domain_conf, conf_line)) {
                std::string line = helpers.trim(conf_line);
                if (line.rfind("LogFormat", 0) == 0) {
                    if (helpers.hasEnding(line, "combinedio")) {
                        helpers.Replace(line, "LogFormat", "");
                        helpers.Replace(line, "combinedio", "");
                        combined_log_config = helpers.trim(line);
                        helpers.Split(conf_array, combined_log_config, " ");
                        logio = true;
                        break;
                    }
                }
            }
        }

        if (!logio) {
//        std::cout << "combinedio isn't active" << std::endl;
            while (getline(domain_conf, conf_line)) {
                std::string line = helpers.trim(conf_line);
                if (line.rfind("LogFormat", 0) == 0) {
                    if (helpers.hasEnding(line, "combined")) {
                        helpers.Replace(line, "LogFormat", "");
                        helpers.Replace(line, "combined", "");
                        combined_log_config = helpers.trim(line);
                        helpers.Split(conf_array, combined_log_config, " ");
                    }
                }
            }
        }

        std::string regex_string;
//        std::cout << "combined_log_config: " << combined_log_config << std::endl;
        for (std::size_t i = 0; i < conf_array.size(); ++i) {
            std::string item = conf_array[i];
            helpers.Replace(item, "\\\"", "");
            helpers.Replace(item, "\"", "");
            if (regex_string.empty()) {
                regex_string = regex_array[item];
            } else {
                regex_string += " " + regex_array[item];
            }
        }

        std::vector<std::string> regex_string_array;
        helpers.Split(regex_string_array, regex_string, " ");
//    std::cout << "regex_string: " << regex_string << std::endl;

        for (std::size_t i = 0; i < regex_string_array.size(); ++i) {
            if (regex_string_array[i].compare("(\\d+)") == 0) {
//                std::cout << "outgoing_byte_index: " << i  << std::endl;
                outgoing_byte_index = i;
            }
            if (regex_string_array[i].compare("[+\\-]\\d{4})\\]") == 0) {
//                std::cout << "time_regex: " << i  << std::endl;
                time_index = i;
            }
        }
        domain_conf.close();
        str_dom_http_regex = regex_string;
//        std::cout << "str_dom_http_regex: " << str_dom_http_regex << std::endl;
    }


    void parseLogFormatForDomain(std::string &system_type, std::string &domain, std::string &httpd_server) {
        if (httpd_server == "apache") {
            parseLogFormatForDomainApache(system_type, domain);
        }
    }

    std::vector<std::string> getApacheArray(std::string &str) {
        std::vector<std::string> result;
        try {
            boost::regex pattern("^" + str_dom_http_regex);
            boost::smatch match;
            if (boost::regex_search(str, match, pattern)) {
                if (!match.empty()) {
                    for (int i = 0; (unsigned) i < match.size(); ++i) {
//                        std::cout << "match-" << i << ":" << match[i] << '\n';
                        result.push_back(match[i]);
                    }
                }
            }
        }
        catch (...) {
//        std::cout << "regex_error caught: " << e.what() << '\n';
//        if (e.code() == std::regex_constants::error_brack) {
//            std::cout << "The code was error_brack\n";
//        }
        }

        return result;
    }

    std::map<std::string, std::string> getNginxArray(std::string &str) {
        std::map<std::string, std::string> result = std::map<std::string, std::string>();
        try {
            const char *json = str.c_str();
            if (!d.Parse(json).HasParseError()) {
                if (d.IsObject()) {
                    if (d.HasMember("msec") && d.HasMember("time_iso8601") && d.HasMember("bytes_sent") &&
                        d.HasMember("http_host")) {
                        result.insert({"timestamp", std::string(d["msec"].GetString())});
                        result.insert({"date", std::string(d["time_iso8601"].GetString())});
                        result.insert({"size", std::string(d["bytes_sent"].GetString())});
                        result.insert({"domain", std::string(d["http_host"].GetString())});
                    }
                }
            }
        }
        catch (...) {
        }

        return result;
    }

    int parseApacheLogFiles(std::string &path) {
        time_t timestamp;
        DIR *dp;
        std::string remove_str = " -0";
        std::string remove_str_s = " +0000";
        struct dirent *dirp;
        if ((dp = opendir(path.c_str())) == nullptr) {
            return -1;
        } else {
            while ((dirp = readdir(dp)) != nullptr) {
                // .gz dosyaları da okunacak
                if (dirp->d_name != std::string(".") && dirp->d_name != std::string("..")) {
                    std::string fileName = path + dirp->d_name + "/";
                    if (!helpers.isDir(path + dirp->d_name)) {
                        std::string file = path + "/" + dirp->d_name;
                        if (helpers.isContains(file, access_log_file_prefix)) {
                            struct stat result_file{};
                            if (stat(file.c_str(), &result_file) == 0) {
                                time_t mod_time = result_file.st_mtime;
                                //. Dosya duzenlenme tarihi eskiyse okumaya gerek yok
                                if (mod_time < from_timestamp) {
                                    continue;
                                }
                            }

                            if (!helpers.isContains(file, gz_prefix)) {
                                std::ifstream log_file(file);
                                std::string line;
                                while (getline(log_file, line)) {
                                    std::vector<std::string> http_result = getApacheArray(line);
                                    if (!http_result.empty()) {
                                        std::string transfer_date = http_result[time_index];
                                        helpers.removeSubStr(transfer_date, remove_str);
                                        helpers.removeSubStr(transfer_date, remove_str_s);
                                        timestamp = helpers.getTimestamp(transfer_date);
                                        if (timestamp > from_timestamp) {
                                            http_traffic += std::stoll(http_result[outgoing_byte_index]);
                                        }
                                    }
                                }
                            } else {
                                igzstream log_file(file.c_str());
                                std::string line;
                                while (getline(log_file, line)) {
                                    std::vector<std::string> http_result = getApacheArray(line);
                                    if (!http_result.empty()) {
                                        std::string transfer_date = http_result[time_index];
                                        helpers.removeSubStr(transfer_date, remove_str);
                                        helpers.removeSubStr(transfer_date, remove_str_s);
                                        timestamp = helpers.getTimestamp(transfer_date);
                                        if (timestamp > from_timestamp) {
                                            http_traffic += std::stoll(http_result[outgoing_byte_index]);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            closedir(dp);
        }
        return 0;
    }

    int parseNginxLogFiles(std::string &path) {
        time_t timestamp;
        DIR *dp;
        std::string remove_str = " -0";
        struct dirent *dirp;
        if ((dp = opendir(path.c_str())) == nullptr) {
            return -1;
        } else {
            while ((dirp = readdir(dp)) != nullptr) {
                // .gz dosyalarıda okunacak
                if (dirp->d_name != std::string(".") && dirp->d_name != std::string("..")) {
                    std::string fileName = path + dirp->d_name + "/";
                    if (!helpers.isDir(path + dirp->d_name)) {
                        std::string file = path + "/" + dirp->d_name;
                        if (helpers.isContains(file, access_log_file_prefix)) {
                            struct stat result_file{};
                            if (stat(file.c_str(), &result_file) == 0) {
                                time_t mod_time = result_file.st_mtime;
                                //. Dosya duzenlenme tarihi eskiyse okumaya gerek yok
                                if (mod_time < from_timestamp) {
                                    continue;
                                }
                            }

                            if (!helpers.isContains(file, gz_prefix)) {
                                std::ifstream log_file(file);
                                std::string line;
                                while (getline(log_file, line)) {
                                    std::map<std::string, std::string> http_result = getNginxArray(line);
                                    if (!http_result.empty()) {
                                        std::string transfer_date = http_result["date"];
                                        timestamp = helpers.fromDateToTimestamp(transfer_date);
                                        if (timestamp > from_timestamp) {
                                            http_traffic += std::stoll(http_result["size"]);
                                        }
                                    }
                                }
                            } else {
                                igzstream log_file(file.c_str());
                                std::string line;
                                while (getline(log_file, line)) {
                                    std::map<std::string, std::string> http_result = getNginxArray(line);
                                    if (!http_result.empty()) {
                                        std::string transfer_date = http_result["date"];
                                        timestamp = helpers.fromDateToTimestamp(transfer_date);
                                        if (timestamp > from_timestamp) {
                                            http_traffic += std::stoll(http_result["size"]);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            closedir(dp);
        }
        return 0;
    }

    int64_t getHttpTraffic() const {
        return int64_t(http_traffic);
    }
};

#endif //FPANEL_STATISTICS_LINUX_HTTP_H
