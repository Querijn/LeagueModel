#pragma once

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = long long;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using f32 = float;
using f64 = double;

using Address = u64; // Assuming x64

#define DerivedFrom(B, A) typename = std::enable_if<std::is_same<A, B>::value == false && std::is_base_of<B, A>::value>

static_assert(sizeof(i8) == 1, "i8 seems to not match the correct size!");
static_assert(sizeof(i16) == 2, "i16 seems to not match the correct size!");
static_assert(sizeof(i32) == 4, "i32 seems to not match the correct size!");
static_assert(sizeof(i64) == 8, "i64 seems to not match the correct size!");

static_assert(sizeof(u8) == 1, "u8 seems to not match the correct size!");
static_assert(sizeof(u16) == 2, "u16 seems to not match the correct size!");
static_assert(sizeof(u32) == 4, "u32 seems to not match the correct size!");
static_assert(sizeof(u64) == 8, "u64 seems to not match the correct size!");

static_assert(sizeof(f32) == 4, "f32 seems to not match the correct size!");
static_assert(sizeof(f64) == 8, "f64 seems to not match the correct size!");
