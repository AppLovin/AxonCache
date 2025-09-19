// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <span>
#include <string>
#include <vector>

namespace axoncache
{
template<typename T>
auto transform( const T & input ) -> std::string;

template<>
auto transform<std::vector<float>>( const std::vector<float> & input ) -> std::string;

template<typename T>
auto transform( std::string_view input ) -> T;

template<>
auto transform<std::vector<float>>( std::string_view input ) -> std::vector<float>;

template<>
auto transform<std::span<const float>>( std::string_view input ) -> std::span<const float>;

}
