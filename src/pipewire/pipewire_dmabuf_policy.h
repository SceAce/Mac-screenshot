#pragma once

namespace markshot::pipewire {

struct DmaBufEnvironment {
    // 会话运行在 KDE Plasma 上
    bool kdeSession = false;
    // 存在 NVIDIA 专有驱动节点
    bool nvidiaProprietaryDriver = false;
    // 系统中 DRM 渲染节点数量
    int renderNodeCount = 0;
    // 环境变量要求强制使用 DMA-BUF
    bool forcedByEnvironment = false;
    // 环境变量要求禁用 DMA-BUF
    bool disabledByEnvironment = false;
    // 本进程内 DMA-BUF 导入已经失败过
    bool importBroken = false;
};

/**
 * 【录制】【PipeWire协商】按环境判断是否应当避开 DMA-BUF 缓冲。
 *
 * KWin 在 NVIDIA 专有驱动上导出 DMA-BUF 会失败，表现为 compositor 侧报
 * GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT，PipeWire 收到无有效数据的缓冲，
 * 录制随即失败。这类组合直接改用共享内存，牺牲零拷贝换取可用。
 *
 * @param environment 环境探测结果。
 * @return 应当避开 DMA-BUF 时返回 true。
 */
bool shouldAvoidDmaBuf(const DmaBufEnvironment &environment);

/**
 * 【录制】【PipeWire协商】探测当前会话环境。
 * @return 环境探测结果。
 */
DmaBufEnvironment currentDmaBufEnvironment();

/**
 * 【录制】【PipeWire协商】按当前会话环境判断是否应当避开 DMA-BUF 缓冲。
 * @return 应当避开 DMA-BUF 时返回 true。
 */
bool shouldAvoidDmaBuf();

/**
 * 【录制】【PipeWire协商】记录一次运行时 DMA-BUF 导入失败。
 *
 * EGL 导入失败往往与具体 compositor/驱动组合相关，重试只会逐帧重复完整的
 * EGL 初始化与 GPU 导入开销并注定失败。标记后，本次进程内的后续流协商
 * 直接改用共享内存。MARK_SHOT_FORCE_DMABUF 环境变量不重置该标记。
 *
 * @return 无返回值。
 */
void markDmaBufImportBroken();

/**
 * 【录制】【PipeWire协商】清除运行时 DMA-BUF 导入失败标记（仅测试使用）。
 * @return 无返回值。
 */
void resetDmaBufImportBrokenForTest();

}  // namespace markshot::pipewire
