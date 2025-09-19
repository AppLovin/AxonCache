// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "ResourceLoader.h"
#include <sys/unistd.h>
#include <unistd.h>
#include <string>

namespace axoncacheTest
{

auto resolveResourcePath( const char * relativePath, const char * currentFile ) -> std::string
{
    const std::string sourcePath( currentFile );
    const auto & parentPath = sourcePath.substr( 0, sourcePath.find_last_of( '/' ) );
    auto resourcePath = parentPath + "/" + relativePath;

    if ( access( resourcePath.c_str(), F_OK ) == -1 )
    {
        std::string errorMsg = "Cannot access file: " + resourcePath;
        throw std::runtime_error( errorMsg.c_str() );
    }

    return resourcePath;
}

}
