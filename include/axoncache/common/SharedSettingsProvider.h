// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <fstream>
#include <map>
#include "axoncache/common/StringUtils.h"

namespace axoncache
{

class SharedSettingsProvider
{
  public:
    explicit SharedSettingsProvider( const std::string & settingsFile )
    {
        // Load settings
        std::ifstream settingsFileStream( settingsFile );

        std::string line;
        while ( std::getline( settingsFileStream, line ) )
        {
            std::string listStr = std::string( line );
            std::vector<std::string> components = StringUtils::splitFirstOccurence( '=', listStr );
            if ( components.size() == 2 )
            {
                std::string key = StringUtils::trim( components.front() );
                std::string value = StringUtils::trim( components.back() );

                // FIXME: support # comments properly
                if ( key.find( '#' ) == std::string::npos )
                {
                    mSettings[key] = value;
                }
            }
        }
    }

    auto getString( const char * name, const char * defaultValue ) const -> std::string
    {
        auto iter = mSettings.find( std::string( name ) );
        if ( iter != mSettings.end() )
        {
            return iter->second;
        }
        else
        {
            return { defaultValue };
        }
    }

    bool getBool( const char * name, const bool defaultValue ) const
    {
        std::string stringValue = getString( name, "" );
        if ( !stringValue.empty() )
        {
            return StringUtils::toBool( stringValue );
        }
        else
        {
            return defaultValue;
        }
    }

    int32_t getInt( const char * name, const int32_t defaultValue ) const
    {
        std::string stringValue = getString( name, "" );
        if ( !stringValue.empty() )
        {
            return StringUtils::toInteger( stringValue );
        }
        else
        {
            return defaultValue;
        }
    }

    double getDouble( const char * name, const double defaultValue ) const
    {
        std::string stringValue = getString( name, "" );
        if ( !stringValue.empty() )
        {
            return StringUtils::toDouble( stringValue );
        }
        else
        {
            return defaultValue;
        }
    }

    char getChar( const char * name, const char defaultValue ) const
    {
        return getChar( std::string{ name }, defaultValue );
    }

    std::string getString( const std::string & name, const char * defaultValue ) const
    {
        return getString( name.c_str(), defaultValue );
    }

    int32_t getInt( const std::string & name, const int32_t defaultValue ) const
    {
        return getInt( name.c_str(), defaultValue );
    }

    char getChar( const std::string & name, char defaultValue ) const
    {
        auto result = getString( name, "" );
        if ( result.empty() )
        {
            return defaultValue;
        }
        else if ( result == R"(\n)" )
        {
            return '\n';
        }
        else if ( result == R"(\t)" )
        {
            return '\t';
        }
        else if ( result == R"(\r)" )
        {
            return '\r';
        }
        else if ( result == R"(\0)" )
        {
            return '\0';
        }
        else if ( result == R"(\b)" )
        {
            return '\b';
        }
        else if ( result == R"(\f)" )
        {
            return '\f';
        }
        else if ( result == R"(\a)" )
        {
            return '\a';
        }
        else if ( result == R"(\v)" )
        {
            return '\v';
        }
        else if ( result == R"(\\)" )
        {
            return '\\';
        }
        else
        {
            return result[0];
        }
    }

    auto getBool( const std::string & name, const bool defaultValue ) const -> bool
    {
        return getBool( name.c_str(), defaultValue );
    }

    auto getDouble( const std::string & name, const double defaultValue ) const -> double
    {
        return getDouble( name.c_str(), defaultValue );
    }

    auto setSetting( const std::string & key, const std::string & value ) -> void
    {
        mSettings[key] = value;
    }

    auto isSet( const std::string & key ) const -> bool
    {
        return mSettings.find( key ) != mSettings.end();
    }

    auto setIfNotSet( const std::string & key, const std::string & value ) -> bool
    {
        if ( !isSet( key ) )
        {
            mSettings[key] = value;
            return true;
        }
        return false;
    }

  private:
    std::map<std::string, std::string> mSettings;
};

}
