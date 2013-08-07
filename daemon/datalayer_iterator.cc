// Copyright (c) 2013, Cornell University
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
//     * Neither the name of HyperDex nor the names of its contributors may be
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

#define __STDC_LIMIT_MACROS

// e
#include <e/endian.h>

// HyperDex
#include "daemon/daemon.h"
#include "daemon/datalayer_encodings.h"
#include "daemon/datalayer_iterator.h"

using hyperdex::datalayer;
using hyperdex::dbwrap_snapshot_ptr;;

namespace
{

int
internal_key_compare(const e::slice& lhs, const e::slice& rhs)
{
    int cmp = memcmp(lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size()));

    if (cmp == 0)
    {
        if (lhs.size() < rhs.size())
        {
            return -1;
        }

        if (lhs.size() > rhs.size())
        {
            return 1;
        }

        return 0;
    }

    return cmp;
}

} // namespace

//////////////////////////////// class iterator ////////////////////////////////

datalayer :: iterator :: iterator(dbwrap_snapshot_ptr s)
    : m_ref(0)
    , m_snap(s)
{
}

dbwrap_snapshot_ptr
datalayer :: iterator :: snap()
{
    return m_snap;
}

datalayer :: iterator :: ~iterator() throw ()
{
}

///////////////////////////// class dummy_iterator /////////////////////////////

datalayer :: dummy_iterator :: dummy_iterator()
    : iterator(dbwrap_snapshot_ptr())
{
}

bool
datalayer :: dummy_iterator :: valid()
{
    return false;
}

void
datalayer :: dummy_iterator :: next()
{
}

uint64_t
datalayer :: dummy_iterator :: cost()
{
    return 0;
}

e::slice
datalayer :: dummy_iterator :: key()
{
    return e::slice();
}

std::ostream&
datalayer :: dummy_iterator :: describe(std::ostream& out) const
{
    return out << "dummy_iterator()";
}

datalayer :: dummy_iterator :: ~dummy_iterator() throw ()
{
}

///////////////////////////// class region_iterator ////////////////////////////

datalayer :: region_iterator :: region_iterator(dbwrap_iterator_ptr iter,
                                                const region_id& ri,
                                                index_info* di)
    : iterator(iter.snap())
    , m_iter(iter)
    , m_ri(ri)
    , m_decoded()
    , m_di(di)
{
    char buf[sizeof(uint8_t) + sizeof(uint64_t)];
    char* ptr = buf;
	MDB_val k = {sizeof(buf), buf};
    ptr = e::pack8be('o', ptr);
    ptr = e::pack64be(ri.get(), ptr);
	mdb_cursor_get(iter.get(), &k, NULL, MDB_SET_RANGE);
}

datalayer :: region_iterator :: ~region_iterator() throw ()
{
}

std::ostream&
datalayer :: region_iterator :: describe(std::ostream& out) const
{
    return out << "region_iterator(" << m_ri << ")";
}

bool
datalayer :: region_iterator :: valid()
{
	MDB_val k;
	int rc;
	
	rc = mdb_cursor_get(m_iter.get(), &k, NULL, MDB_GET_CURRENT);
	if (rc)
    {
        return false;
    }

    if (k.mv_size < sizeof(uint8_t) + sizeof(uint64_t))
    {
        return false;
    }

    if (*(const char *)k.mv_data != 'o')
    {
        return false;
    }

    uint64_t ri;
    e::unpack64be((const char *)k.mv_data + sizeof(uint8_t), &ri);
    return region_id(ri) == m_ri;
}

void
datalayer :: region_iterator :: next()
{
	mdb_cursor_get(m_iter.get(), NULL, NULL, MDB_NEXT);
}

uint64_t
datalayer :: region_iterator :: cost()
{
	MDB_txn *txn;
	MDB_cursor *mc;
	MDB_val k1, k2;
    uint64_t ret = 0;
	MDB_dbi dbi;
	int rc;
    const size_t sz = sizeof(uint8_t) + sizeof(uint64_t);
    char buf[2 * sz];
    char* ptr = buf;
    ptr = e::pack8be('o', ptr);
    ptr = e::pack64be(m_ri.get(), ptr);
    ptr = e::pack8be('o', ptr);
    ptr = e::pack64be(m_ri.get(), ptr);
    encode_bump(buf + sz, buf + 2 * sz);
    // create the range
	k1.mv_data = buf;
	k1.mv_size = sz;
	k2.mv_data = buf + sz;
	k2.mv_size = sz;
	txn = mdb_cursor_txn(m_iter.get());
	dbi = mdb_cursor_dbi(m_iter.get());
	// tally up the sizes of all the keys in [k1,k2)
	rc = mdb_cursor_open(txn, dbi, &mc);
	rc = mdb_cursor_get(mc, &k1, NULL, MDB_SET_RANGE);
	while (rc == MDB_SUCCESS) {
		if (mdb_cmp(txn, dbi, &k1, &k2) > 0)
			break;
		ret += k1.mv_size;
		rc = mdb_cursor_get(mc, &k1, NULL, MDB_NEXT);
	}
	mdb_cursor_close(mc);
    return ret;
}

e::slice
datalayer :: region_iterator :: key()
{
    const size_t sz = sizeof(uint8_t) + sizeof(uint64_t);
	MDB_val _k;
	mdb_cursor_get(m_iter.get(), &_k, NULL, MDB_GET_CURRENT);
    e::slice k = e::slice((const char *)_k.mv_data + sz, _k.mv_size - sz);
    size_t decoded_sz = m_di->decoded_size(k);

    if (m_decoded.size() < decoded_sz)
    {
        m_decoded.resize(decoded_sz);
    }

    m_di->decode(k, &m_decoded.front());
    return e::slice(&m_decoded.front(), decoded_sz);
}

///////////////////////////// class index_iterator /////////////////////////////

datalayer :: index_iterator :: index_iterator(dbwrap_snapshot_ptr s)
    : iterator(s)
{
}

datalayer :: index_iterator :: ~index_iterator() throw ()
{
}

//////////////////////////// class intersect_iterator ////////////////////////////

datalayer :: intersect_iterator :: intersect_iterator(dbwrap_snapshot_ptr s,
                                                      const std::vector<e::intrusive_ptr<index_iterator> >& iterators)
    : index_iterator(s)
    , m_iters()
    , m_cost(0)
    , m_invalid(false)
{
    assert(!iterators.empty());
    std::vector<std::pair<uint64_t, e::intrusive_ptr<index_iterator> > > iters;

    for (size_t i = 0; i < iterators.size(); ++i)
    {
        assert(iterators[i]->sorted());
        iters.push_back(std::make_pair(iterators[i]->cost(), iterators[i]));
    }

    std::sort(iters.begin(), iters.end());
    m_iters.resize(iters.size());

    for (size_t i = 0; i < iters.size(); ++i)
    {
        m_cost += iters[i].first;
        m_iters[i] = iters[i].second;
    }

    for (size_t i = 0; i < m_iters.size(); ++i)
    {
        if (!m_iters[i]->valid())
        {
            m_invalid = true;
        }
    }
}

datalayer :: intersect_iterator :: ~intersect_iterator() throw ()
{
}

bool
datalayer :: intersect_iterator :: valid()
{
    while (!m_invalid && m_iters[0]->valid())
    {
        bool retry = false;

        for (size_t i = 1; i < m_iters.size(); ++i)
        {
            m_iters[i]->seek(m_iters[0]->internal_key());

            if (!m_iters[i]->valid())
            {
                m_invalid = true;
                return false;
            }

            int cmp = internal_key_compare(m_iters[0]->internal_key(),
                                           m_iters[i]->internal_key());

            if (cmp < 0)
            {
                m_iters[0]->seek(m_iters[i]->internal_key());
                retry = true;
                break;
            }

            assert(cmp == 0);
        }

        if (!retry)
        {
            return true;
        }
    }

    m_invalid = true;
    return false;
}

void
datalayer :: intersect_iterator :: next()
{
    m_iters[0]->next();
}

uint64_t
datalayer :: intersect_iterator :: cost()
{
    return m_cost;
}

e::slice
datalayer :: intersect_iterator :: key()
{
    return m_iters[0]->key();
}

std::ostream&
datalayer :: intersect_iterator :: describe(std::ostream& out) const
{
    out << "intersect_iterator(";

    for (size_t i = 0; i < m_iters.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }

        out << *m_iters[i];
    }

    return out << ")";
}

e::slice
datalayer :: intersect_iterator :: internal_key()
{
    return m_iters[0]->internal_key();
}

bool
datalayer :: intersect_iterator :: sorted()
{
    return true;
}

void
datalayer :: intersect_iterator :: seek(const e::slice& k)
{
    return m_iters[0]->seek(k);
}

///////////////////////////// class search_iterator ////////////////////////////

datalayer :: search_iterator :: search_iterator(datalayer* dl,
                                                const region_id& ri,
                                                e::intrusive_ptr<index_iterator> iter,
                                                std::ostringstream* ostr,
                                                const std::vector<attribute_check>* checks)
    : iterator(iter->snap())
    , m_dl(dl)
    , m_ri(ri)
    , m_iter(iter)
    , m_error(SUCCESS)
    , m_ostr(ostr)
    , m_num_gets(0)
    , m_checks(checks)
{
}

datalayer :: search_iterator :: ~search_iterator() throw ()
{
}

std::ostream&
datalayer :: search_iterator :: describe(std::ostream& out) const
{
    return out << "search_iterator(" << *m_iter << ")";
}

bool
datalayer :: search_iterator :: valid()
{
    if (m_error != SUCCESS)
    {
        return false;
    }

    // Don't try to optimize by replacing m_ri with a const schema* because it
    // won't persist across reconfigurations
    const schema& sc(*m_dl->m_daemon->m_config.get_schema(m_ri));

    uint64_t version;
    std::vector<e::slice> value;
    reference ref;

    // while the most selective iterator is valid and not past the end
    while (m_iter->valid())
    {
		MDB_txn *txn;
		MDB_val k, val;
        std::vector<char> kbacking;
        DB_SLICE lkey;
		int rc;
        encode_key(m_ri, sc.attrs[0].type, m_iter->key(), &kbacking, &lkey);
		MVSL(k,lkey);

		txn = m_iter->snap().get();
		rc = mdb_get(txn, 1, &k, &val);

        if (rc == MDB_SUCCESS)
        {
            e::slice v((const char *)val.mv_data, val.mv_size);
            rc = decode_value(v, &value, &version);

            if (rc != SUCCESS)
            {
                m_error = (datalayer::returncode)rc;
                return false;
            }

            ++m_num_gets;
        }
        else
        {
            m_error = m_dl->handle_error(rc);
            return false;
        }

        if (passes_attribute_checks(sc, *m_checks, m_iter->key(), value) == m_checks->size())
        {
            return true;
        }
        else
        {
            m_iter->next();
        }
    }

    if (m_ostr) *m_ostr << " iterator retrieved " << m_num_gets << " objects from disk\n";
    return false;
}

void
datalayer :: search_iterator :: next()
{
    m_iter->next();
}

uint64_t
datalayer :: search_iterator :: cost()
{
    return m_iter->cost();
}

e::slice
datalayer :: search_iterator :: key()
{
    return m_iter->key();
}
