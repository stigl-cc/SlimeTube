/**
 *
 *  Authenticator.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;


class Authenticator : public HttpFilter<Authenticator>
{
  public:
    Authenticator() {}
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};

