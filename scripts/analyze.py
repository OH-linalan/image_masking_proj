from collections import defaultdict
import os
from pathlib import Path
import re
import matplotlib.pyplot as plt
import numpy as np


def clean_op_name(name: str) -> str:
    """파일명 및 경로 파라미터(: input/...)를 제거하여 순수 연산 이름으로 정규화"""
    if " : " in name:
        name = name.split(" : ")[0].strip()
    return name


def parse_and_analyze_log():
    # 기준 경로 설정 (현재 스크립트 기준)
    base_dir = Path(__file__).resolve().parent.parent
    log_file_path = base_dir / "output" / "log.txt"
    output_dir = Path(__file__).resolve().parent / "output"

    output_dir.mkdir(parents=True, exist_ok=True)

    if not log_file_path.exists():
        print(f"오류: 로그 파일을 찾을 수 없습니다 -> {log_file_path}")
        return

    pairs = []
    rmse_list = []
    iou_list = []
    total_time_list = []
    fast_time_list = []
    brief_time_list = []

    operation_times = defaultdict(list)

    re_frame = re.compile(r"processing frame : (scene_\d+-\d+ -> scene_\d+-\d+)")
    re_op_time = re.compile(r"\[DEBUG\] \[(.+?)\] Execution time: \d+ us \(([\d\.]+) ms\)")
    re_rmse = re.compile(r"RMSE between images: ([\d\.]+)")
    re_iou = re.compile(r"IOU between masks: ([\d\.]+)")

    current_pair = None
    current_rmse = None
    current_iou = None
    current_total = None
    current_fast = None
    current_brief = None

    with open(log_file_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            m_frame = re_frame.search(line)
            if m_frame:
                if current_pair and current_total is not None:
                    pairs.append(current_pair)
                    rmse_list.append(current_rmse if current_rmse is not None else 0.0)
                    iou_list.append(current_iou if current_iou is not None else 0.0)
                    total_time_list.append(current_total)
                    fast_time_list.append(current_fast if current_fast is not None else 0.0)
                    brief_time_list.append(current_brief if current_brief is not None else 0.0)

                current_pair = m_frame.group(1)
                current_rmse = None
                current_iou = None
                current_total = None
                current_fast = None
                current_brief = None
                continue

            m_op = re_op_time.search(line)
            if m_op:
                raw_op_name = m_op.group(1).strip()
                ms_val = float(m_op.group(2))

                if not raw_op_name.startswith("Total Execution Time"):
                    normalized_op_name = clean_op_name(raw_op_name)
                    operation_times[normalized_op_name].append(ms_val)

                if "Total Execution Time" in raw_op_name:
                    current_total = ms_val
                elif "cvFeature: FAST Detect" in raw_op_name:
                    current_fast = ms_val
                elif "cvFeature: BRIEF Compute" in raw_op_name:
                    current_brief = ms_val
                continue

            m_rmse = re_rmse.search(line)
            if m_rmse and current_rmse is None:
                current_rmse = float(m_rmse.group(1))
                continue

            m_iou = re_iou.search(line)
            if m_iou and current_iou is None:
                current_iou = float(m_iou.group(1))
                continue

    if current_pair and current_total is not None:
        pairs.append(current_pair)
        rmse_list.append(current_rmse if current_rmse is not None else 0.0)
        iou_list.append(current_iou if current_iou is not None else 0.0)
        total_time_list.append(current_total)
        fast_time_list.append(current_fast if current_fast is not None else 0.0)
        brief_time_list.append(current_brief if current_brief is not None else 0.0)

    if not pairs:
        print("경고: 유효한 페어 데이터를 추출하지 못했습니다.")
        return

    # 1. 종합 통계 연산
    avg_total_ms = np.mean(total_time_list)
    std_total_ms = np.std(total_time_list)
    min_total_ms = np.min(total_time_list)
    max_total_ms = np.max(total_time_list)
    avg_fps = 1000.0 / avg_total_ms if avg_total_ms > 0 else 0.0

    avg_iou, std_iou = np.mean(iou_list), np.std(iou_list)
    min_iou, max_iou = np.min(iou_list), np.max(iou_list)

    avg_rmse, std_rmse = np.mean(rmse_list), np.std(rmse_list)
    min_rmse, max_rmse = np.min(rmse_list), np.max(rmse_list)

    # 2. 각 연산별 통계 계산 (평균, 표준편차, 최소, 최대, 호출 횟수)
    op_stats = []
    for op, times in operation_times.items():
        arr = np.array(times)
        avg_t = np.mean(arr)
        std_t = np.std(arr)
        min_t = np.min(arr)
        max_t = np.max(arr)
        count = len(arr)
        op_stats.append({
            "name": op,
            "mean": avg_t,
            "std": std_t,
            "min": min_t,
            "max": max_t,
            "count": count
        })

    # 평균 소요시간 기준 내림차순 정렬
    op_stats.sort(key=lambda x: x["mean"], reverse=True)
    slowest_op = op_stats[0]

    # 세부 연산별 통계 표 텍스트 구성
    table_header = f"{'연산 이름 (Operation)':<52} | {'평균 (ms)':<10} | {'표준편차':<10} | {'최소 (ms)':<10} | {'최대 (ms)':<10} | {'호출 횟수':<8}"
    divider = "-" * len(table_header)
    
    table_rows = [table_header, divider]
    for stat in op_stats:
        row = f"{stat['name']:<52} | {stat['mean']:>9.3f}  | {stat['std']:>9.3f}  | {stat['min']:>9.3f}  | {stat['max']:>9.3f}  | {stat['count']:>7}회"
        table_rows.append(row)
    op_table_text = "\n".join(table_rows)

    # 전체 리포트 생성
    report_text = f"""========================================================================================================================
                                             [ 벤치마크 및 연산별 성능 통계 보고서 ]
========================================================================================================================
총 처리된 페어 수        : {len(pairs)} 쌍
총 실행 시간 (Total)     : 평균 {avg_total_ms:.2f} ms (±{std_total_ms:.2f} ms) | 최소 {min_total_ms:.2f} ms | 최대 {max_total_ms:.2f} ms ({avg_fps:.2f} FPS)
가장 오래 걸리는 연산    : [{slowest_op['name']}] (평균 {slowest_op['mean']:.3f} ms)

[ 마스크 IOU 평가 ]
- 평균 (표준편차)        : {avg_iou:.4f} (±{std_iou:.4f})
- 최소 / 최대 IOU       : {min_iou:.4f} / {max_iou:.4f}

[ 이미지 RMSE 평가 ]
- 평균 (표준편차)        : {avg_rmse:.4f} (±{std_rmse:.4f})
- 최소 / 최대 RMSE      : {min_rmse:.4f} / {max_rmse:.4f}

------------------------------------------------------------------------------------------------------------------------
                                         [ 각 연산별 세부 통계 (Mean, Std, Min, Max) ]
------------------------------------------------------------------------------------------------------------------------
{op_table_text}
========================================================================================================================
"""
    print(report_text)
    with open(output_dir / "summary_report.txt", "w", encoding="utf-8") as f:
        f.write(report_text)

    indices = np.arange(1, len(pairs) + 1)
    plt.rcParams["font.family"] = "DejaVu Sans"

    # 1. IOU & RMSE 추이 그래프
    fig, ax1 = plt.subplots(figsize=(12, 5), dpi=300)
    color = "tab:blue"
    ax1.set_xlabel("Frame Pair Index")
    ax1.set_ylabel("IOU", color=color)
    ax1.plot(indices, iou_list, color=color, linewidth=1.5, label="IOU")
    ax1.tick_params(axis="y", labelcolor=color)
    ax1.set_ylim(0, 1.05)
    ax1.grid(True, linestyle="--", alpha=0.5)

    ax2 = ax1.twinx()
    color = "tab:red"
    ax2.set_ylabel("RMSE", color=color)
    ax2.plot(indices, rmse_list, color=color, linewidth=1.5, linestyle="--", label="RMSE")
    ax2.tick_params(axis="y", labelcolor=color)

    plt.title("IOU & RMSE Metric Trends per Frame Pair")
    fig.tight_layout()
    plt.savefig(output_dir / "metric_trends.png")
    plt.close()

    # 2. 총 실행시간 추이 그래프
    plt.figure(figsize=(12, 5), dpi=300)
    plt.plot(indices, total_time_list, color="tab:green", linewidth=1.5, label="Total Time (ms)")
    plt.axhline(avg_total_ms, color="red", linestyle=":", label=f"Average ({avg_total_ms:.2f} ms)")
    plt.title("Total Execution Time per Pair")
    plt.xlabel("Frame Pair Index")
    plt.ylabel("Execution Time (ms)")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_dir / "total_execution_time.png")
    plt.close()

    # 3. FAST vs BRIEF 실행시간 비교 그래프
    plt.figure(figsize=(12, 5), dpi=300)
    plt.plot(indices, fast_time_list, color="tab:purple", linewidth=1.5, label="FAST Detect (ms)")
    plt.plot(indices, brief_time_list, color="tab:orange", linewidth=1.5, label="BRIEF Compute (ms)")
    plt.title("FAST Detect vs BRIEF Compute Execution Time")
    plt.xlabel("Frame Pair Index")
    plt.ylabel("Execution Time (ms)")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_dir / "feature_extraction_time.png")
    plt.close()

    # 4. 연산별 통계 막대그래프 (평균 + 표준편차 에러바 + Min/Max Scatter 점)
    labels = [stat["name"] for stat in op_stats][::-1]
    means = [stat["mean"] for stat in op_stats][::-1]
    stds = [stat["std"] for stat in op_stats][::-1]
    mins = [stat["min"] for stat in op_stats][::-1]
    maxs = [stat["max"] for stat in op_stats][::-1]
    y_pos = np.arange(len(labels))

    fig_height = max(7, len(op_stats) * 0.45)
    plt.figure(figsize=(13, fig_height), dpi=300)

    # 평균 막대 및 표준편차 오차 막대
    bars = plt.barh(y_pos, means, xerr=stds, align='center', alpha=0.75, 
                    color='royalblue', edgecolor='navy', ecolor='dimgray', capsize=4, label='Mean ± Std')

    # Min / Max 포인트 표시
    plt.scatter(mins, y_pos, color='forestgreen', s=35, zorder=5, marker='|', label='Min')
    plt.scatter(maxs, y_pos, color='crimson', s=35, zorder=5, marker='|', label='Max')

    plt.yticks(y_pos, labels, fontsize=9)
    plt.xlabel("Execution Time (ms)", fontsize=10)
    plt.title("Operation Execution Time Statistics (Mean, Std, Min, Max)", fontsize=12, pad=15)
    plt.grid(True, linestyle="--", alpha=0.5, axis="x")
    plt.legend(loc='lower right', frameon=True)

    # 텍스트 레이블 (평균값 표시)
    for i, bar in enumerate(bars):
        w = bar.get_width()
        plt.text(w + stds[i] + (max(means) * 0.015), bar.get_y() + bar.get_height() / 2,
                 f"{w:.2f} ms", va="center", ha="left", fontsize=8.5, color='black')

    plt.tight_layout()
    plt.savefig(output_dir / "operation_stats_chart.png")
    plt.close()

    print(f"모든 통계 및 그래프 파일 생성 완료 -> {output_dir}")


if __name__ == "__main__":
    parse_and_analyze_log()