/*! Common tools and definitions suitable for general purposes.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include "AnalysisTools/Core/include/Tools.h"
#include <boost/regex.hpp>
// #include <regex>
#include <boost/crc.hpp>
#include <boost/filesystem.hpp>

namespace analysis {

namespace tools {

uint32_t hash(const std::string& str)
{
    boost::crc_32_type crc;
    crc.process_bytes(str.data(), str.size());
    return crc.checksum();
}

std::vector<std::string> FindFiles(const std::string& path, const std::string& file_name_pattern)
{
    using directory_iterator = boost::filesystem::directory_iterator;

    std::vector<std::string> all_files;
    for (const auto& dir_entry : directory_iterator(path)){
        std::string n_path = ToString(dir_entry);
        std::string file_name = GetFileNameWithoutPath(n_path);
        all_files.push_back(file_name);
    }
    boost::regex pattern (file_name_pattern);
    std::vector<std::string> names_matched;
    for(size_t n = 0; n < all_files.size(); n++){
        if(regex_match(all_files.at(n), pattern))
            names_matched.push_back(all_files.at(n));
    }
    return names_matched;
}

} // namespace tools
} // namespace analysis
