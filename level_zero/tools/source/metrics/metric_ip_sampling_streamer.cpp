/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/tools/source/metrics/metric_ip_sampling_streamer.h"

#include "level_zero/core/source/device/device.h"
#include "level_zero/tools/source/metrics/metric.h"
#include "level_zero/tools/source/metrics/metric_ip_sampling_source.h"
#include "level_zero/tools/source/metrics/os_interface_metric.h"
#include <level_zero/zet_api.h>

#include <set>
#include <string.h>

namespace L0 {

ze_result_t IpSamplingMetricGroupImp::streamerOpen(
    zet_context_handle_t hContext,
    zet_device_handle_t hDevice,
    zet_metric_streamer_desc_t *desc,
    ze_event_handle_t hNotificationEvent,
    zet_metric_streamer_handle_t *phMetricStreamer) {

    auto device = Device::fromHandle(hDevice);

    // Check whether metric group is activated.
    IpSamplingMetricSourceImp &source = device->getMetricDeviceContext().getMetricSource<IpSamplingMetricSourceImp>();
    if (!source.isMetricGroupActivated(this->toHandle())) {
        return ZE_RESULT_NOT_READY;
    }

    // Check whether metric streamer is already open.
    if (source.pActiveStreamer != nullptr) {
        return ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE;
    }

    auto pStreamerImp = new IpSamplingMetricStreamerImp(source);
    UNRECOVERABLE_IF(pStreamerImp == nullptr);

    const ze_result_t result = source.getMetricOsInterface()->startMeasurement(desc->notifyEveryNReports, desc->samplingPeriod);
    if (result == ZE_RESULT_SUCCESS) {
        source.pActiveStreamer = pStreamerImp;
        pStreamerImp->attachEvent(hNotificationEvent);
    } else {
        delete pStreamerImp;
        pStreamerImp = nullptr;
        return result;
    }

    *phMetricStreamer = pStreamerImp->toHandle();
    return ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricStreamerImp::readData(uint32_t maxReportCount, size_t *pRawDataSize, uint8_t *pRawData) {

    // Return required size if requested.
    if (*pRawDataSize == 0) {
        *pRawDataSize = ipSamplingSource.getMetricOsInterface()->getRequiredBufferSize(maxReportCount);
        return ZE_RESULT_SUCCESS;
    }

    // If there is a difference in pRawDataSize and maxReportCount, use the minimum value for reading.
    if (maxReportCount != UINT32_MAX) {
        size_t maxSizeRequired = ipSamplingSource.getMetricOsInterface()->getRequiredBufferSize(maxReportCount);
        *pRawDataSize = std::min(maxSizeRequired, *pRawDataSize);
    }

    return ipSamplingSource.getMetricOsInterface()->readData(pRawData, pRawDataSize);
}

ze_result_t IpSamplingMetricStreamerImp::close() {

    const ze_result_t result = ipSamplingSource.getMetricOsInterface()->stopMeasurement();
    detachEvent();
    ipSamplingSource.pActiveStreamer = nullptr;
    delete this;

    return result;
}

Event::State IpSamplingMetricStreamerImp::getNotificationState() {

    return ipSamplingSource.getMetricOsInterface()->isNReportsAvailable()
               ? Event::State::STATE_SIGNALED
               : Event::State::STATE_INITIAL;
}

uint32_t IpSamplingMetricStreamerImp::getMaxSupportedReportCount() {
    const auto unitReportSize = ipSamplingSource.getMetricOsInterface()->getUnitReportSize();
    UNRECOVERABLE_IF(unitReportSize == 0);
    return ipSamplingSource.getMetricOsInterface()->getRequiredBufferSize(UINT32_MAX) / unitReportSize;
}

ze_result_t IpSamplingMetricCalcOpImp::create(IpSamplingMetricSourceImp &metricSource,
                                              zet_intel_metric_calculate_exp_desc_t *pCalculateDesc,
                                              bool isMultiDevice,
                                              zet_intel_metric_calculate_operation_exp_handle_t *phCalculateOperation) {

    // There is only one valid metric group in IP sampling and time filtering is not supported
    // So only metrics handles are used to filter the metrics

    // avoid duplicates
    std::set<zet_metric_handle_t> uniqueMetricHandles(pCalculateDesc->phMetrics, pCalculateDesc->phMetrics + pCalculateDesc->metricCount);

    // The order of metrics in the report should be the same as the one in the HW report to optimize calculation
    uint32_t metricGroupCount = 1;
    zet_metric_group_handle_t hMetricGroup = {};
    metricSource.metricGroupGet(&metricGroupCount, &hMetricGroup);
    uint32_t metricCount = 0;
    MetricGroup::fromHandle(hMetricGroup)->metricGet(&metricCount, nullptr);
    std::vector<zet_metric_handle_t> hMetrics(metricCount);
    MetricGroup::fromHandle(hMetricGroup)->metricGet(&metricCount, hMetrics.data());
    std::vector<MetricImp *> inputMetricsInReport = {};

    for (uint32_t i = 0; i < metricCount; i++) {
        auto metric = static_cast<MetricImp *>(Metric::fromHandle(hMetrics[i]));
        if (pCalculateDesc->metricGroupCount > 0) {
            inputMetricsInReport.push_back(metric);
        } else {
            if (uniqueMetricHandles.find(hMetrics[i]) != uniqueMetricHandles.end()) {
                inputMetricsInReport.push_back(metric);
            }
        }
    }

    auto calcOp = new IpSamplingMetricCalcOpImp(inputMetricsInReport, isMultiDevice);
    *phCalculateOperation = calcOp->toHandle();
    return ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricCalcOpImp::destroy() {
    delete this;
    return ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricCalcOpImp::getReportFormat(uint32_t *pCount, zet_metric_handle_t *phMetrics) {

    if (*pCount == 0) {
        *pCount = metricCount;
        return ZE_RESULT_SUCCESS;
    } else if (*pCount < metricCount) {
        METRICS_LOG_ERR("%s", "Metric can't be smaller than report size");
        *pCount = 0;
        return ZE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    *pCount = metricCount;
    for (uint32_t index = 0; index < metricCount; index++) {
        phMetrics[index] = metricsInReport[index]->toHandle();
    }

    return ZE_RESULT_SUCCESS;
}

ze_result_t MultiDeviceIpSamplingMetricGroupImp::streamerOpen(
    zet_context_handle_t hContext,
    zet_device_handle_t hDevice,
    zet_metric_streamer_desc_t *desc,
    ze_event_handle_t hNotificationEvent,
    zet_metric_streamer_handle_t *phMetricStreamer) {

    ze_result_t result = ZE_RESULT_SUCCESS;

    std::vector<IpSamplingMetricStreamerImp *> subDeviceStreamers = {};
    subDeviceStreamers.reserve(subDeviceMetricGroup.size());

    // Open SubDevice Streamers
    for (auto &metricGroup : subDeviceMetricGroup) {
        zet_metric_streamer_handle_t subDeviceMetricStreamer = {};
        zet_device_handle_t hSubDevice = metricGroup->getMetricSource().getMetricDeviceContext().getDevice().toHandle();
        result = metricGroup->streamerOpen(hContext, hSubDevice, desc, nullptr, &subDeviceMetricStreamer);
        if (result != ZE_RESULT_SUCCESS) {
            closeSubDeviceStreamers(subDeviceStreamers);
            return result;
        }
        subDeviceStreamers.push_back(static_cast<IpSamplingMetricStreamerImp *>(MetricStreamer::fromHandle(subDeviceMetricStreamer)));
    }

    auto pStreamerImp = new MultiDeviceIpSamplingMetricStreamerImp(subDeviceStreamers);
    UNRECOVERABLE_IF(pStreamerImp == nullptr);

    pStreamerImp->attachEvent(hNotificationEvent);
    *phMetricStreamer = pStreamerImp->toHandle();
    return result;
}

ze_result_t MultiDeviceIpSamplingMetricStreamerImp::readData(uint32_t maxReportCount, size_t *pRawDataSize, uint8_t *pRawData) {

    const int32_t totalHeaderSize = static_cast<int32_t>(sizeof(IpSamplingMetricDataHeader) * subDeviceStreamers.size());
    // Find single report size
    size_t singleReportSize = 0;
    subDeviceStreamers[0]->readData(1, &singleReportSize, nullptr);

    // Trim report count to the maximum possible report count
    const uint32_t maxSupportedReportCount = subDeviceStreamers[0]->getMaxSupportedReportCount() *
                                             static_cast<uint32_t>(subDeviceStreamers.size());
    maxReportCount = std::min(maxSupportedReportCount, maxReportCount);

    if (*pRawDataSize == 0) {
        *pRawDataSize = singleReportSize * maxReportCount;
        *pRawDataSize += totalHeaderSize;
        return ZE_RESULT_SUCCESS;
    }

    // Remove header size from actual data size
    size_t calcRawDataSize = std::max<int32_t>(0, static_cast<int32_t>(*pRawDataSize - totalHeaderSize));

    // Recalculate maximum possible report count for the raw data size
    calcRawDataSize = std::min(calcRawDataSize, singleReportSize * maxReportCount);
    maxReportCount = static_cast<uint32_t>(calcRawDataSize / singleReportSize);
    uint8_t *pCurrRawData = pRawData;
    size_t currRawDataSize = calcRawDataSize;

    ze_result_t result = ZE_RESULT_SUCCESS;

    for (uint32_t index = 0; index < subDeviceStreamers.size(); index++) {
        auto &streamer = subDeviceStreamers[index];

        // Get header address
        auto header = reinterpret_cast<IpSamplingMetricDataHeader *>(pCurrRawData);
        pCurrRawData += sizeof(IpSamplingMetricDataHeader);

        result = streamer->readData(maxReportCount, &currRawDataSize, pCurrRawData);
        if (result != ZE_RESULT_SUCCESS) {
            *pRawDataSize = 0;
            return result;
        }

        // Update to header
        memset(header, 0, sizeof(IpSamplingMetricDataHeader));
        header->magic = IpSamplingMetricDataHeader::magicValue;
        header->rawDataSize = static_cast<uint32_t>(currRawDataSize);
        header->setIndex = index;

        calcRawDataSize -= currRawDataSize;
        pCurrRawData += currRawDataSize;

        // Check whether memory available for next read
        if (calcRawDataSize < singleReportSize) {
            break;
        }
        maxReportCount -= static_cast<uint32_t>(currRawDataSize / singleReportSize);
        currRawDataSize = calcRawDataSize;
    }

    *pRawDataSize = pCurrRawData - pRawData;
    return result;
}

ze_result_t MultiDeviceIpSamplingMetricStreamerImp::close() {

    ze_result_t result = ZE_RESULT_SUCCESS;
    for (auto &streamer : subDeviceStreamers) {
        result = streamer->close();
        if (result != ZE_RESULT_SUCCESS) {
            break;
        }
    }

    subDeviceStreamers.clear();
    detachEvent();
    delete this;
    return result;
}

Event::State MultiDeviceIpSamplingMetricStreamerImp::getNotificationState() {

    Event::State state = Event::State::STATE_INITIAL;
    for (auto &streamer : subDeviceStreamers) {
        state = streamer->getNotificationState();
        if (state == Event::State::STATE_SIGNALED) {
            break;
        }
    }

    return state;
}

} // namespace L0
