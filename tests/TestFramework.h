#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace ctg::test
{
    struct TestCase
    {
        std::string name;
        std::function<void()> func;
    };

    inline std::vector<TestCase>& Registry()
    {
        static std::vector<TestCase> tests;
        return tests;
    }

    inline void Register(const std::string& name, std::function<void()> func)
    {
        Registry().push_back(TestCase{ name, std::move(func) });
    }

    inline int RunAll()
    {
        int failed = 0;
        int total = 0;

        for (const auto& t : Registry())
        {
            ++total;
            try
            {
                t.func();
                std::cout << "[ OK ] " << t.name << "\n";
            }
            catch (const std::exception& ex)
            {
                ++failed;
                std::cout << "[FAIL] " << t.name << " - " << ex.what() << "\n";
            }
            catch (...)
            {
                ++failed;
                std::cout << "[FAIL] " << t.name << " - unknown error\n";
            }
        }

        std::cout << "Ran " << total << " tests, failures: " << failed << "\n";
        return failed == 0 ? 0 : 1;
    }

    [[noreturn]] inline void Fail(const std::string& message)
    {
        throw std::runtime_error(message);
    }
}

#define CTG_TEST(name)                                                        \
    static void name();                                                       \
    namespace {                                                               \
        struct name##_registrar {                                             \
            name##_registrar() { ::ctg::test::Register(#name, name); }        \
        } name##_registrar_instance;                                          \
    }                                                                         \
    static void name()

#define CTG_EXPECT_TRUE(cond)                                                 \
    do                                                                        \
    {                                                                         \
        if (!(cond))                                                          \
        {                                                                     \
            ::ctg::test::Fail(std::string("EXPECT_TRUE failed: ") + #cond);   \
        }                                                                     \
    } while (false)

#define CTG_EXPECT_EQ(lhs, rhs)                                               \
    do                                                                        \
    {                                                                         \
        auto _lhs = (lhs);                                                    \
        auto _rhs = (rhs);                                                    \
        if (!((_lhs) == (_rhs)))                                              \
        {                                                                     \
            ::ctg::test::Fail(std::string("EXPECT_EQ failed: ") + #lhs        \
                              + " == " + #rhs);                               \
        }                                                                     \
    } while (false)

