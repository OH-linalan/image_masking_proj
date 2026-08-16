import os
import re
import subprocess
import cv2
import numpy as np

# --- 1. 파일 및 텍스트 검증 함수 ---

def compare_images(img1_path, img2_path):
    """두 이미지 파일이 완벽히 동일한지 픽셀 단위로 비교"""
    if not os.path.exists(img1_path) or not os.path.exists(img2_path):
        print(f"❌ [FAIL] 파일 없음: {img1_path} 또는 {img2_path}")
        return False

    img1 = cv2.imread(img1_path)
    img2 = cv2.imread(img2_path)

    if img1.shape != img2.shape:
        print(f"❌ [FAIL] 크기 불일치: {img1_path} {img1.shape} vs {img2.shape}")
        return False

    diff = cv2.absdiff(img1, img2)
    diff_sum = np.sum(diff)

    if diff_sum == 0:
        print(f"✅ [PASS] 이미지 완벽 일치: {os.path.basename(img1_path)}")
        return True
    else:
        print(f"⚠️ [DIFF] 이미지 차이 발생 (Diff Sum: {diff_sum}): {os.path.basename(img1_path)}")
        return False


def parse_stdout(stdout_text):
    """터미널 출력에서 수치 결과값 추출"""
    metrics = {}
    
    # Diff 배열 수치 추출
    diff_match = re.search(r"Diff\s*:\s*\[([\d\s,]+)\]", stdout_text)
    if diff_match:
        metrics['diff'] = [int(x.strip()) for x in diff_match.group(1).split(',')]

    # RMSE 추출
    rmse_match = re.search(r"RMSE between images:\s*([\d.]+)", stdout_text)
    if rmse_match:
        metrics['rmse'] = float(rmse_match.group(1))

    # IoU 추출
    iou_match = re.search(r"IOU between masks:\s*([\d.]+)", stdout_text)
    if iou_match:
        metrics['iou'] = float(iou_match.group(1))

    return metrics


# --- 2. 메인 검증 프로세스 ---

def main():
    print("=" * 60)
    print("🚀 C++ 실행 결과 검증 스크립트")
    print("=" * 60)

    # 1) 프로그램 실행 및 터미널 출력 캡처
    cmd = ["./output/a.out"]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=False)
        stdout = result.stdout
    except FileNotFoundError:
        print("❌ [ERROR] ./output/a.out 파일을 찾을 수 없습니다. 'make'를 먼저 실행하세요.")
        return

    print("\n[1] 수치 결과(Metrics) 검증")
    metrics = parse_stdout(stdout)

    # 기준값 설정 (제공해주신 로그 기준)
    expected_rmse = 35.6272
    expected_iou = 0.972517

    # RMSE 검증 (소수점 오차 감안)
    if 'rmse' in metrics:
        if np.isclose(metrics['rmse'], expected_rmse, atol=1e-3):
            print(f"✅ [PASS] RMSE 일치: {metrics['rmse']} (기준: {expected_rmse})")
        else:
            print(f"❌ [FAIL] RMSE 불일치: {metrics['rmse']} (기준: {expected_rmse})")
    else:
        print("❌ [FAIL] RMSE 수치를 로그에서 찾을 수 없습니다.")

    # IoU 검증
    if 'iou' in metrics:
        if np.isclose(metrics['iou'], expected_iou, atol=1e-3):
            print(f"✅ [PASS] IoU 일치: {metrics['iou']} (기준: {expected_iou})")
        else:
            print(f"❌ [FAIL] IoU 불일치: {metrics['iou']} (기준: {expected_iou})")
    else:
        print("❌ [FAIL] IoU 수치를 로그에서 찾을 수 없습니다.")

    print("\n[2] 생성된 이미지 파일 존재 여부 확인")
    output_files = [
        "output/first_masked_output.bmp",
        "output/second_masked_output_firstdata.bmp",
        "output/second_masked_output_homography.bmp",
        "output/second_masked_output_homography_opencv.bmp",
        "output/ORB_matches.png",
        "output/ORB_accepted_matches.png",
        "output/homography_difference.png",
        "output/first_warped_Hcam.bmp",
        "output/first_warped_Hcv.bmp"
    ]

    all_exist = True
    for filepath in output_files:
        if os.path.exists(filepath):
            print(f"  - {filepath} ... OK")
        else:
            print(f"  - {filepath} ... MISSING ❌")
            all_exist = False

    if all_exist:
        print("\n🎉 모든 이미지 결과물이 정상적으로 생성되었습니다!")

if __name__ == "__main__":
    main()