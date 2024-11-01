#pragma once
#include <ydb/core/tx/columnshard/common/portion.h>
#include <ydb/core/tx/columnshard/common/snapshot.h>
#include <ydb/core/tx/columnshard/engines/protos/portion_info.pb.h>
#include <ydb/library/formats/arrow/replace_key.h>
#include <ydb/core/formats/arrow/special_keys.h>
#include <ydb/library/accessor/accessor.h>
#include <util/stream/output.h>

namespace NKikimr::NOlap {

struct TIndexInfo;

struct TPortionMeta {
private:
    NArrow::TFirstLastSpecialKeys ReplaceKeyEdges; // first and last PK rows
    YDB_READONLY_DEF(TString, TierName);
    YDB_READONLY(ui32, DeletionsCount, 0);
    YDB_READONLY(ui32, CompactionLevel, 0);
    YDB_READONLY(ui32, RecordsCount, 0);
    YDB_READONLY(ui64, ColumnRawBytes, 0);
    YDB_READONLY(ui32, ColumnBlobBytes, 0);
    YDB_READONLY(ui32, IndexRawBytes, 0);
    YDB_READONLY(ui32, IndexBlobBytes, 0);

    friend class TPortionMetaConstructor;
    friend class TPortionInfo;
    TPortionMeta(NArrow::TFirstLastSpecialKeys& pk, const TSnapshot& min, const TSnapshot& max)
        : ReplaceKeyEdges(pk)
        , RecordSnapshotMin(min)
        , RecordSnapshotMax(max)
        , IndexKeyStart(pk.GetFirst())
        , IndexKeyEnd(pk.GetLast())
    {
        AFL_VERIFY(IndexKeyStart <= IndexKeyEnd)("start", IndexKeyStart.DebugString())("end", IndexKeyEnd.DebugString());
    }
    TSnapshot RecordSnapshotMin;
    TSnapshot RecordSnapshotMax;

public:
    const NArrow::TFirstLastSpecialKeys& GetFirstLastPK() const {
        return ReplaceKeyEdges;
    }

    void ResetCompactionLevel(const ui32 level) {
        CompactionLevel = level;
    }

    using EProduced = NPortion::EProduced;

    NArrow::TReplaceKey IndexKeyStart;
    NArrow::TReplaceKey IndexKeyEnd;

    EProduced Produced = EProduced::UNSPECIFIED;

    std::optional<TString> GetTierNameOptional() const;

    ui64 GetMetadataMemorySize() const {
        return sizeof(TPortionMeta) + ReplaceKeyEdges.GetMemorySize();
    }

    NKikimrTxColumnShard::TIndexPortionMeta SerializeToProto() const;

    EProduced GetProduced() const {
        return Produced;
    }

    TString DebugString() const;
};

class TPortionAddress {
private:
    YDB_READONLY(ui64, PathId, 0);
    YDB_READONLY(ui64, PortionId, 0);
public:
    TPortionAddress(const ui64 pathId, const ui64 portionId)
        : PathId(pathId)
        , PortionId(portionId)
    {

    }

    TString DebugString() const;

    bool operator<(const TPortionAddress& item) const {
        return std::tie(PathId, PortionId) < std::tie(item.PathId, item.PortionId);
    }

    bool operator==(const TPortionAddress& item) const {
        return std::tie(PathId, PortionId) == std::tie(item.PathId, item.PortionId);
    }
};

} // namespace NKikimr::NOlap

template<>
struct THash<NKikimr::NOlap::TPortionAddress> {
    inline ui64 operator()(const NKikimr::NOlap::TPortionAddress& x) const noexcept {
        return CombineHashes(x.GetPortionId(), x.GetPathId());
    }
};

