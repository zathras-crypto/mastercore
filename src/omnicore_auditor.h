#ifndef OMNICORE_AUDITOR_H
#define OMNICORE_AUDITOR_H

#include <stdint.h>
#include <string>

class CBlockIndex;

extern bool auditorEnabled;

namespace mastercore
{
  void Auditor_Initialize();
  void Auditor_NotifyBlockStart(CBlockIndex const * pBlockIndex);
  void Auditor_NotifyBlockFinish(CBlockIndex const * pBlockIndex);
  void Auditor_NotifyPropertyTotalChanged(bool incrase, uint32_t propertyId, int64_t amount, std::string const& reasonStr);
  void Auditor_NotifyPropertyCreated(uint32_t propertyId);
}

uint32_t ComparePropertyTotals();
bool ComparePropertyCounts();

#endif // OMNICORE_AUDITOR_H



