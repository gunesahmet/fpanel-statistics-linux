#pragma once
#ifndef FPANEL_STATISTICS_LINUX_DISK_H
#define FPANEL_STATISTICS_LINUX_DISK_H

class Disk
{
private:
    unsigned long long disk_httpd = 0;
    unsigned long long disk_mail = 0;
    unsigned long long disk_log = 0;
    Helpers helpers;
public:
    int64_t getHttpDisk() const {
        return int64_t(disk_httpd);
    }

    int64_t getMailDisk() const {
        return int64_t(disk_mail);
    }

    int64_t getLogDisk() const {
        return int64_t(disk_log);
    }

    void calculateHttpdocDisk(std::string &domain) {
        std::string vhost_path = "/var/www/" + domain + "/vhost";
        disk_httpd = helpers.getFolderSize(vhost_path.c_str());
    }

    void calculateMailDisk(std::string &domain) {
        std::string path = "/usr/local/fpanel/mail/domains/" + domain;
        disk_mail = helpers.getFolderSize(path.c_str());
    }

    void calculateLogDisk(std::string &domain) {
        std::string dom_http_log_path = "/var/log/fpanel/domains/";
        dom_http_log_path.append(domain);
        dom_http_log_path.append("/web");
        disk_log = helpers.getFolderSize(dom_http_log_path.c_str());
    }
};

#endif //FPANEL_STATISTICS_LINUX_DISK_H
