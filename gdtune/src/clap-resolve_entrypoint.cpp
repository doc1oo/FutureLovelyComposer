/*
 * CLAP-INFO
 *
 * https://github.com/free-audio/clap-info
 *
 * CLAP-INFO is Free and Open Source software, released under the MIT
 * License, a copy of which is included with this source in the file
 * "LICENSE.md"
 *
 * Copyright (c) 2022 Various Authors, per the Git Transaction Log
 */

#include "clap-scanner.h"
#include <iostream>

#include <windows.h>
#include <shlobj_core.h>

namespace clap_scanner
{

const clap_plugin_entry_t *entryFromCLAPPath(const std::filesystem::path &p)
{
    auto han = LoadLibrary((LPCSTR)(p.generic_string().c_str()));
    if (!han)
        return nullptr;
    auto phan = GetProcAddress(han, "clap_entry");
    //    std::cout << "phan is " << phan << std::endl;
    return (clap_plugin_entry_t *)phan;
}


std::vector<std::filesystem::path> validCLAPSearchPaths()
{
    std::vector<std::filesystem::path> res;

    {
        // I think this should use SHGetKnownFilderLocation but I don't know windows well enough
        auto p = getenv("COMMONPROGRAMFILES");
        if (p)
        {
            res.emplace_back(std::filesystem::path{p} / "CLAP");
        }
        auto q = getenv("LOCALAPPDATA");
        if (q)
        {
            res.emplace_back(std::filesystem::path{q} / "Programs" / "Common" / "CLAP");
        }
    }

    auto p = getenv("CLAP_PATH");

    if (p)
    {
        auto sep = ';';
        auto cp = std::string(p);

        size_t pos;
        while ((pos = cp.find(sep)) != std::string::npos)
        {
            auto item = cp.substr(0, pos);
            cp = cp.substr(pos + 1);
            res.emplace_back(std::filesystem::path{item});
        }
        if (cp.size())
            res.emplace_back(std::filesystem::path{cp});
    }

    return res;
}

} // namespace clap_scanner
