#include "service_advertiser.h"

#include "service_common.h"

#include <iostream>
#include <netdb.h>
#include <sys/select.h>

namespace sppvip {

namespace {

void
registerCallback(DNSServiceRef sdRef,
                 DNSServiceFlags flags,
                 DNSServiceErrorType ec,
                 const char* name,
                 const char* regtype,
                 const char* domain,
                 void* context)
{
    std::cout << "Service registration completed." << std::endl
              << "Advertising " << name << "." << domain << std::endl;
}

}

service_advertiser::service_advertiser()
    : is_advertising_(false)
    , sppvip_reg_srv_()
    , advertise_thread_(nullptr)
{}

service_advertiser::~service_advertiser()
{
    if (advertise_thread_ && advertise_thread_->joinable()) {

        if (is_advertising_) {
            std::scoped_lock<decltype(advertise_mutex_)> lock(advertise_mutex_);
            is_advertising_ = false;
        }

        advertise_thread_->join();
    }

    DNSServiceRefDeallocate(sppvip_reg_srv_);
}

bool
service_advertiser::advertise() noexcept
{
    if (is_advertising_) {
        return false;
    }

    using namespace std::string_literals;

    auto s = "test"s;
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, s.size(), s.data());

    auto err = DNSServiceRegister(&sppvip_reg_srv_,
                                  0,
                                  0,
                                  NULL,
                                  SPPVIP_SERVICE_NAME,
                                  NULL,
                                  NULL,
                                  htons(SPPVIP_PORT),
                                  TXTRecordGetLength(&txtRecord),
                                  &txtRecord,
                                  registerCallback,
                                  this);

    if (!err) {
        is_advertising_ = true;

        advertise_thread_ = std::make_unique<decltype(
            advertise_thread_)::element_type>([this]() {
            int sockFd = DNSServiceRefSockFD(sppvip_reg_srv_);
            int nSockFd = sockFd + 1;

            while (true) {
                fd_set rfds;
                struct timeval readTv;

                FD_ZERO(&rfds);
                FD_SET(sockFd, &rfds);

                readTv.tv_sec = 1;
                readTv.tv_usec = 0;

                int retVal = select(nSockFd, &rfds, NULL, NULL, &readTv);

                DNSServiceErrorType err = kDNSServiceErr_NoError;
                if (retVal > 0) {
                    if (FD_ISSET(sockFd, &rfds)) {
                        err = DNSServiceProcessResult(sppvip_reg_srv_);
                    }
                }

                if (err) {
                    std::cout
                        << "Failed processing service advertising results: "
                        << mdns_error_string(err) << std::endl;
                    break;
                }

                std::cout << "trying to get advr mtx" << std::endl;
                std::scoped_lock<decltype(advertise_mutex_)> lock(
                    advertise_mutex_);
                std::cout << "got adverstise lock" << std::endl;
                if (!is_advertising_) {
                    break;
                }
            }

            std::cout << "Done advertising" << std::endl;
        });
    }

    return !err;
}

void
service_advertiser::stop()
{
    std::scoped_lock<decltype(advertise_mutex_)> lock(advertise_mutex_);
    is_advertising_ = false;
}

};
