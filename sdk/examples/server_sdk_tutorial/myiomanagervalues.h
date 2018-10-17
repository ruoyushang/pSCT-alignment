#ifndef __MYIOMANAGERVALUE_H__
#define __MYIOMANAGERVALUE_H__

#include "iomanager.h"
#include "opcuatypesinternal.h"
#include "uathread.h"
#include "mytreenodebase.h"

class MyTree;
class TreeSampleGroupItem;

/** Class for the information about a sampled Variable value.
 *
 * This is the representation of a monitored item in the IOManager
 */
class TreeSampleItem
{
    UA_DISABLE_COPY(TreeSampleItem);
public:
    TreeSampleItem(TreeSampleGroupItem* pSampleGroupItem, IOVariableCallback* pIOVariableCallback, OpcUa_TimestampsToReturn timestampsToReturn);
    ~TreeSampleItem();

    inline void setTreeSampleGroupItem(TreeSampleGroupItem* pItem){m_pSampleGroupItem=pItem;}
    inline TreeSampleGroupItem* pSampleGroupItem(){return m_pSampleGroupItem;}
    inline IOVariableCallback* pIOVariableCallback(){return m_pIOVariableCallback;}

    void dataChange(const UaVariant& currentValue);
    void statusChange(const UaStatus& newStatus);

private:
    TreeSampleGroupItem*     m_pSampleGroupItem;
    IOVariableCallback*      m_pIOVariableCallback;
    OpcUa_TimestampsToReturn m_timestampsToReturn;
};

/** Class for a Variable in a SampleGroup
 *
 * This class links the sampling items for one sample interval to a variable in the address space.
 */
class TreeSampleGroupItem
{
    UA_DISABLE_COPY(TreeSampleGroupItem);
public:
    TreeSampleGroupItem(MyVariableHandleOffset* pVariableHandle, UaVariant initialValue);
    ~TreeSampleGroupItem();

    void addSampleItem(TreeSampleItem* pSampleItem);
    void removeSampleItem(TreeSampleItem* pSampleItem);
    inline OpcUa_UInt32 sampleItemCount(){return (OpcUa_UInt32)m_sampleItems.size();};

    inline MyVariableHandleOffset* pVariableHandle(){return m_pVariableHandle;}

    inline void setGroupIndex(OpcUa_UInt16 groupIndex){m_groupIndex = groupIndex;}
    inline OpcUa_UInt16 groupIndex(){return m_groupIndex;}

    void dataChange(const UaVariant& currentValue);
    void statusChange(const UaStatus& newStatus);
    inline void setWriteChangeFlag(){m_writeChangeFlag=true;}

private:
    MyVariableHandleOffset*    m_pVariableHandle;
    std::list<TreeSampleItem*> m_sampleItems;
    OpcUa_UInt16               m_groupIndex;
    UaVariant                  m_lastValue;
    OpcUa_StatusCode           m_lastStatus;
    bool                       m_writeChangeFlag;
};

/** Class for a sample group in the IOManager representing a sampling interval
 */
class SampleGroup
{
public:
    SampleGroup();
    ~SampleGroup();

    void addGroupItem(TreeSampleGroupItem* pGroupItem);
    void removeGroupItem(TreeSampleGroupItem* pGroupItem);
    inline OpcUa_UInt32 sampleGroupItemCount(){return (OpcUa_UInt32)m_sampleGroupItems.size();};
    TreeSampleGroupItem* getFirstSampleGroupItem();
    TreeSampleGroupItem* getNextSampleGroupItem();

    inline void setSamplingInterval(OpcUa_Double samplingInterval){m_samplingInterval = samplingInterval;}
    inline OpcUa_Double samplingInterval(){return m_samplingInterval;}

    void setBaseSamplingInterval(OpcUa_Double baseSamplingInterval);
    bool checkSampling();

private:
    std::list<TreeSampleGroupItem*> m_sampleGroupItems;
    std::list<TreeSampleGroupItem*>::iterator m_currentSampleGroupItem;
    OpcUa_Double                    m_samplingInterval;
    OpcUa_UInt16                    m_maxCounter;
    OpcUa_UInt16                    m_currentCounter;
};

/** Implements the IOMaanger interface for the Value attribute of Variable tree nodes
 *
 */
class MyIOManagerValues :
    public IOManager,
    public UaThread
{
    UA_DISABLE_COPY(MyIOManagerValues);
    MyIOManagerValues(){}
public:
    MyIOManagerValues(MyTree* pTree);
    virtual ~MyIOManagerValues();

    //- Interface IOManager ------------------------------------------------------------------
    UaStatus beginTransaction(IOManagerCallback* pCallback, const ServiceContext& serviceContext, OpcUa_UInt32 hTransaction, OpcUa_UInt32 totalItemCountHint,
        OpcUa_Double maxAge, OpcUa_TimestampsToReturn timestampsToReturn, TransactionType transactionType, OpcUa_Handle& hIOManagerContext);
    UaStatus beginStartMonitoring(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, IOVariableCallback* pIOVariableCallback,
        VariableHandle* pVariableHandle, MonitoringContext& monitoringContext);
    UaStatus beginModifyMonitoring(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, OpcUa_UInt32 hIOVariable, MonitoringContext& monitoringContext);
    UaStatus beginStopMonitoring(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, OpcUa_UInt32 hIOVariable);
    UaStatus beginRead(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, VariableHandle* pVariableHandle, OpcUa_ReadValueId* pReadValueId);
    UaStatus beginWrite(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, VariableHandle* pVariableHandle, OpcUa_WriteValue* pWriteValue);
    UaStatus finishTransaction(OpcUa_Handle hIOManagerContext);
    //- Interface IOManager ------------------------------------------------------------------

    //- Interface UaThread -------------------------------------------------------------------
    void run();
    //- Interface UaThread -------------------------------------------------------------------

private:
    UaMutex                        m_mutex;
    MyTree*                        m_pTree;
    HandleManager<TreeSampleItem>  m_handlesSampleItems;
    bool                           m_isStopped;
    SampleGroup                    m_sampleGroups[MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT];
};

#endif // __MYIOMANAGERVALUE_H__