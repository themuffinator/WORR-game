// version.hpp (Shared Version Information)
// Centralized definitions for the WORR title and dynamically generated build
// version. The version string may be injected by the build system so that
// runtime commands (for example the `gameversion` console command) and logs
// report the exact build that produced the binary.

#pragma once

#include <string_view>

namespace worr::version {

// Human-friendly name for the mod. Shared between the client and the server so
// text shown to the user remains consistent across modules.
inline constexpr std::string_view kGameTitle{"WORR"};

namespace detail {

#if defined(WORR_GENERATED_VERSION_STRING)
inline constexpr std::string_view kVersionSource{WORR_GENERATED_VERSION_STRING};
#elif defined(WORR_GENERATED_VERSION)
inline constexpr std::string_view kVersionSource{WORR_GENERATED_VERSION};
#elif defined(WORR_VERSION_STRING)
inline constexpr std::string_view kVersionSource{WORR_VERSION_STRING};
#elif defined(WORR_VERSION)
inline constexpr std::string_view kVersionSource{WORR_VERSION};
#else
// Fallback used when the build system has not injected a version yet. This
// keeps local development builds compiling while still providing a sensible
// default value.
inline constexpr std::string_view kVersionSource{"0.60.10 BETA dev"};
#endif

}  // namespace detail

// Full display version for the running build. Downstream code should rely on
// this constant so that version reporting is consistent everywhere.
inline constexpr std::string_view kGameVersion = detail::kVersionSource;

}  // namespace worr::version

