#pragma once

#include <fstream>
#include <string>

namespace env
{

/** @class Env
 *  @brief Overridable std::getenv interface
 */
struct Env
{
    virtual ~Env() = default;

    virtual const char* get(const char* key) const = 0;
};

/** @class EnvImpl
 *  @brief Concrete implementation that calls std::getenv
 */
struct EnvImpl : public Env
{
    const char* get(const char* key) const override;
};

/** @brief Default instantiation of Env */
extern EnvImpl env_impl;

/** @brief Reads an environment variable
 *
 *  Reads the environment for that key
 *
 *  @param[in] key - the key
 *  @param[in] env - env interface that defaults to calling std::getenv
 *
 *  @return string - the env var value
 */
inline std::string getEnv(const char* key, const Env* env = &env_impl)
{
    // Be aware value could be nullptr
    auto value = env->get(key);
    return (value) ? std::string(value) : std::string();
}

} // namespace env
