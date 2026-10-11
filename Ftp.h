#pragma once
#ifndef FPANEL_STATISTICS_LINUX_FTP_H
#define FPANEL_STATISTICS_LINUX_FTP_H
std::regex str_dom_ftp_regex(
        "(\\w{3} \\w{3} \\d{2} \\d{2}:\\d{2}:\\d{2} \\d{4}) (\\d*) (\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b) (\\d*) (\\S*) ([a|b]) (\\S{1}) (\\S{1}) (\\S{1}) (\\S*) (\\S*) (\\S*) (\\S*) (\\S*)");
//regex str_dom_http_regex("^([\\d.]+) (\\S+) (\\S+) \\[([\\w:/]+\\s[+\\-]\\d{4})\\] \"(.+?)\" (\\d{3}) (\\d+) \"([^\"]+)\" \"([^\"]+)\"");

class Ftp
{
private:
    time_t from_timestamp;
    std::string gz_prefix = ".gz";
    unsigned long long ftp_traffic = 0;
    Helpers helpers;

public:
    void setFromTimestamp(time_t ft) {
        from_timestamp = ft;
    }

    int64_t getFtpTraffic() {
        return int64_t(ftp_traffic);
    }

    std::vector<std::string> getFtpArray(std::string &str) {
        std::vector<std::string> result;
//    try {
        std::smatch match;
        if (regex_search(str, match, str_dom_ftp_regex)) {
            if (!match.empty()) {
                if (match.size() > 11) {
                    for (int i = 0; (unsigned) i < match.size(); ++i) {
                        result.push_back(match[i]);
                    }
                }
            }
        }
//    }
//    catch (const std::regex_error& e) {
//        std::cout << "regex_error caught: " << e.what() << '\n';
//        if (e.code() == std::regex_constants::error_brack) {
//            std::cout << "The code was error_brack\n";
//        }
//    }

        return result;
    }


    int parseFtpLogFiles(std::string &path, std::string &domain) {
        struct tm tm;
        time_t timestamp;
        std::string line = "";
        std::string xfer_prefix = "xferlog";
        DIR *dp;
        std::string domain_str = "/" + domain + "/";

        struct dirent *dirp;
        if ((dp = opendir(path.c_str())) == NULL) {
            return -1;
        } else {
            while ((dirp = readdir(dp)) != NULL) {
                if (dirp->d_name != std::string(".") && dirp->d_name != std::string("..")) {
                    std::string file = path + dirp->d_name;
                    if (!helpers.isDir(file)) {
                        struct stat result_file;
                        if (stat(file.c_str(), &result_file) == 0) {
                            int mod_time = result_file.st_mtime;
                            if (mod_time < from_timestamp)
                                continue;
                        }
                        if (helpers.isContains(file, xfer_prefix)) {
                            if (!helpers.isContains(file, gz_prefix)) {
                                std::ifstream ftp_log_file(file);
                                while (getline(ftp_log_file, line)) {
                                    std::vector<std::string> ftp_result = getFtpArray(line);
                                    if (ftp_result.size() > 0) {
                                        std::string transfer_type = ftp_result[8];

                                        if (helpers.isContains(line, domain_str)) {
                                            if (transfer_type.compare("o") == 0) {
                                                std::string transfer_date = ftp_result[1];
                                                strptime(transfer_date.c_str(), "%a %b %d %H:%M:%S %Y", &tm);

                                                timestamp = mktime(&tm);
                                                // Tarih verilen tarihten sonra ise size'i devreye al
                                                if (timestamp > from_timestamp) {
//                                            std::cout << timestamp << std::endl;
                                                    std::string ftp_size_r = ftp_result[4];
                                                    if (helpers.is_number(ftp_size_r))
                                                        ftp_traffic += std::stoll(ftp_size_r);
                                                }

                                            }
                                        }
                                    }

                                }
                            } else {
                                // cout << "ftp gz okunuyor" << endl;
                                igzstream ftp_log_file(file.c_str());
                                while (getline(ftp_log_file, line)) {
                                    std::vector<std::string> ftp_result = getFtpArray(line);
                                    if (ftp_result.size() > 0) {
                                        std::string transfer_type = ftp_result[8];
                                        if (helpers.isContains(line, domain_str)) {
                                            if (transfer_type.compare("o") == 0) {
                                                std::string transfer_date = ftp_result[1];
                                                strptime(transfer_date.c_str(), "%a %b %d %H:%M:%S %Y", &tm);

                                                timestamp = mktime(&tm);
                                                // Tarih verilen tarihten sonra ise size'i devreye al
                                                if (timestamp > from_timestamp) {
                                                    std::string ftp_size_r = ftp_result[4];
                                                    if (helpers.is_number(ftp_size_r))
                                                        ftp_traffic += std::stoll(ftp_size_r);
                                                }
                                            }
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
        // 1. kolon tarih
        // 2. kolon
        // 3. kolon client ip
        // 4. kolon transfer boyutu
        // 5. kolon dosya
        // 6. kolon
        // 7. kolon
        // 8. kolon transfer türü
        // 9. kolon
        // 10. kolon
        // 11. kolon
        // 12. kolon
        // 13. kolon
        // 14. kolon
    }

};

#endif //FPANEL_STATISTICS_LINUX_FTP_H
