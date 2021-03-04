#include "env.hpp"

#include <cstdlib>
#include <string>

namespace env
{

const char* EnvImpl::get(const char* key) const
{
    return std::getenv(key);
}

EnvImpl env_impl;

} // namespace env
