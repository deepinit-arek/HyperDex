// Copyright (c) 2012, Cornell University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Replicant nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// C
#include <cstdlib>

// po6
#include <po6/error.h>

// e
#include <e/guard.h>

// HyperDex
#include "client/coordinator_link.h"
#include "client/hyperclient.hpp"
#include "tools/common.h"

static struct poptOption popts[] = {
    POPT_AUTOHELP
    CONNECT_TABLE
    POPT_TABLEEND
};

int
main(int argc, const char* argv[])
{
    poptContext poptcon;
    poptcon = poptGetContext(NULL, argc, argv, popts, POPT_CONTEXT_POSIXMEHARDER);
    e::guard g = e::makeguard(poptFreeContext, poptcon); g.use_variable();
    poptSetOtherOptionHelp(poptcon, "[OPTIONS] <region> <server>");
    int rc;

    while ((rc = poptGetNextOpt(poptcon)) != -1)
    {
        switch (rc)
        {
            case 'h':
                if (!check_host())
                {
                    return EXIT_FAILURE;
                }
                break;
            case 'p':
                if (!check_port())
                {
                    return EXIT_FAILURE;
                }
                break;
            case POPT_ERROR_NOARG:
            case POPT_ERROR_BADOPT:
            case POPT_ERROR_BADNUMBER:
            case POPT_ERROR_OVERFLOW:
                std::cerr << poptStrerror(rc) << " " << poptBadOption(poptcon, 0) << std::endl;
                return EXIT_FAILURE;
            case POPT_ERROR_OPTSTOODEEP:
            case POPT_ERROR_BADQUOTE:
            case POPT_ERROR_ERRNO:
            default:
                std::cerr << "logic error in argument parsing" << std::endl;
                return EXIT_FAILURE;
        }
    }

    const char** args = poptGetArgs(poptcon);
    size_t num_args = 0;

    while (args && args[num_args])
    {
        ++num_args;
    }

    if (num_args != 2)
    {
        std::cerr << "command takes two arguments" << std::endl;
        poptPrintUsage(poptcon, stderr, 0);
        return EXIT_FAILURE;
    }

    try
    {
        hyperdex::coordinator_link cl(po6::net::hostname(_connect_host, _connect_port));
        char* end;
        end = const_cast<char*>(args[0]);
        uint64_t rid = strtoull(args[0], &end, 0);

        if (args[0] == '\0' || *end != '\0')
        {
            std::cerr << "first argument must be a region identifier" << std::endl;
            return EXIT_FAILURE;
        }

        end = const_cast<char*>(args[1]);
        uint64_t sid = strtoull(args[1], &end, 0);

        if (args[1] == '\0' || *end != '\0')
        {
            std::cerr << "first argument must be a region identifier" << std::endl;
            return EXIT_FAILURE;
        }

        hyperclient_returncode e = cl.initiate_transfer(rid, sid);

        if (e != HYPERCLIENT_SUCCESS)
        {
            std::cerr << "could not initiate transfer: " << e << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
    catch (po6::error& e)
    {
        std::cerr << "system error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
