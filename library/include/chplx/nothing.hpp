//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/type_support/unused.hpp>

namespace chplx {

using nothing = hpx::util::unused_type;
inline constexpr nothing none = hpx::util::unused;
} // namespace chplx
