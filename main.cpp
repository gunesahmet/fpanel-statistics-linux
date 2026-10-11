#include <iostream>
#include <fstream>
#include <regex>

#include <sys/stat.h>

#include <dirent.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "argvparser.h"
#include "gzstream.h"


#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define BOOST_NO_CXX11_SCOPED_ENUMS

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#undef BOOST_NO_CXX11_SCOPED_ENUMS

#include "Helpers.h"
#include "Disk.h"
#include "Ftp.h"
#include "Http.h"

#define __int64 long long

using namespace rapidjson;
using namespace CommandLineProcessing;

time_t from_timestamp;


Ftp ftp;
Http http;
Disk disk;
Helpers helpers;

void calculateDomainStatistics(std::string &domain, std::string &system, std::string &httpd_server) {
    std::string dom_http_log_path = "/var/log/fpanel/domains/";
    dom_http_log_path.append(domain);
    dom_http_log_path.append("/web");
    std::string ftp_log_path = "/var/log/proftpd/";

    ftp.parseFtpLogFiles(ftp_log_path, domain);
    if (httpd_server == "nginx") {
        http.parseNginxLogFiles(dom_http_log_path);
    } else {
        http.parseApacheLogFiles(dom_http_log_path);
    }

    disk.calculateHttpdocDisk(domain);
    disk.calculateMailDisk(domain);
    disk.calculateLogDisk(domain);
}

int main(int argc, char *argv[]) {
    struct tm tm{0};
    ArgvParser cmd;
    cmd.defineOption("domain", "", ArgvParser::OptionRequired);
    cmd.defineOption("user", "", ArgvParser::OptionRequired);
    cmd.defineOption("system", "", ArgvParser::OptionRequired);
    cmd.defineOption("httpd", "", ArgvParser::OptionRequired);
    cmd.defineOption("prefix", "", ArgvParser::OptionRequired);
    cmd.defineOption("time");

    int result = cmd.parse(argc, argv);

    if (result != ArgvParser::NoParserError) {
        std::cout << cmd.parseErrorDescription(result) << std::endl;
        exit(1);
    }

    std::string domain = cmd.optionValue("domain");

    std::string from_time = cmd.optionValue("time");
    std::string username = cmd.optionValue("user");
    std::string system = cmd.optionValue("system");
    std::string httpd_server = cmd.optionValue("httpd");

    if (!from_time.empty() && from_time != "0") {
        std::stringstream geek(from_time);
        int x = 0;
        geek >> x;
        from_timestamp = x;
    } else {
//        from_time = "01/01/2016:00:00:00";
//        strptime(from_time.c_str(), "%d/%m/%Y:%H:%M:%S", &tm);
        from_timestamp = std::stoi(from_time);
    }

    ftp.setFromTimestamp(from_timestamp);
    http.setFromTimestamp(from_timestamp);

    http.parseLogFormatForDomain(system, domain, httpd_server);
    http.parseLogFormatForDefaultVhost(system, httpd_server);
    calculateDomainStatistics(domain, system, httpd_server);

    StringBuffer s_json;
    Writer<StringBuffer> writer(s_json);

    writer.StartObject();
    writer.Key("http_traffic");
    writer.Int64(http.getHttpTraffic());

    writer.Key("ftp_traffic");
    writer.Int64(ftp.getFtpTraffic());

    writer.Key("disk_httpd");
    writer.Int64(disk.getHttpDisk());

    writer.Key("disk_mail");
    writer.Int64(disk.getMailDisk());

    writer.Key("disk_log");
    writer.Int64(disk.getLogDisk());

    writer.Key("last_scan");
    writer.String(helpers.currentDateTime().c_str());

    writer.EndObject();

    std::cout << s_json.GetString() << std::endl;
    return 0;
}
