/*
 * Fimex
 *
 * (C) Copyright 2011, met.no
 *
 * Project Info:  https://wiki.met.no/fimex/start
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */


// internals
//
#include "../../include/metgm/MetGmTags.h"
#include "../../include/metgm/MetGmDimensionsTag.h"
#include "../../include/metgm/MetGmGroup1Ptr.h"
#include "../../include/metgm/MetGmGroup3Ptr.h"
#include "../../include/metgm/MetGmFileHandlePtr.h"
#include "../../include/metgm/MetGmHandlePtr.h"
#include "../../include/metgm/MetGmUtils.h"

#include "metgm.h"

namespace MetNoFimex {

    boost::shared_ptr<MetGmHandlePtr> MetGmHandlePtr::createMetGmHandleForReading(const std::string& source)
    {
        boost::shared_ptr<MetGmHandlePtr> pHandle = boost::shared_ptr<MetGmHandlePtr>(new MetGmHandlePtr);

        pHandle->pFileHandle_ = MetGmFileHandlePtr::createMetGmFileHandlePtrForReading((source));
        if(!(pHandle->pFileHandle_.get()))
            throw CDMException(std::string("error opening metgm file handle for: ") + source);

        MGM_THROW_ON_ERROR(mgm_read_header(*pHandle->pFileHandle_, *pHandle));

        pHandle->pVersion_ = MetGmVersion::createMetGmVersion(mgm_get_version(*pHandle));

        return pHandle;
    }

    boost::shared_ptr<MetGmHandlePtr> MetGmHandlePtr::createMetGmHandleForWriting(boost::shared_ptr<MetGmFileHandlePtr>& pFileHandle,
                                                                                  boost::shared_ptr<MetGmVersion>& pVersion)
    {
        assert(pFileHandle.get());
        assert(pVersion.get());

        boost::shared_ptr<MetGmHandlePtr> pHandle = boost::shared_ptr<MetGmHandlePtr>(new MetGmHandlePtr);

        assert(pHandle.get());

        pHandle->pFileHandle_ = pFileHandle;
        pHandle->pVersion_    = pVersion;

        return pHandle;
    }

    MetGmHandlePtr::~MetGmHandlePtr()
    {
        MGM_THROW_ON_ERROR(mgm_free_handle(handle_));
    }
}
