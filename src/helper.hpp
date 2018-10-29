#pragma once

#define DerivedFrom(B, A) typename = std::enable_if<std::is_same<A, B>::value == false && std::is_base_of<B, A>::value>
#define AssertDerivedFrom(B, A, Message) static_assert(std::is_same<A, B>::value == false && std::is_base_of<B, A>::value, Message);