import os
import re
from pathlib import Path
from ultralytics import YOLO


def natural_sort_key(file_path: Path):
    """파일명 내부의 숫자를 기준으로 자연스럽게 정렬 (예: scene_01-2 -> scene_01-10)"""
    return [
        int(text) if text.isdigit() else text.lower()
        for text in re.split(r"(\d+)", file_path.stem)
    ]


def batch_detect_and_save_boxes():
    # 기준 경로 설정 (현재 스크립트 기준 상위 디렉터리)
    base_dir = Path(__file__).resolve().parent.parent

    # 입력 및 출력 디렉터리 경로 설정
    scene_dir = base_dir / "input" / "scene"
    output_dir = base_dir / "input" / "boxes"

    # 출력 디렉터리가 없으면 자동 생성
    output_dir.mkdir(parents=True, exist_ok=True)

    # .bmp 이미지 파일 검색 및 정렬
    image_files = sorted(list(scene_dir.glob("*.bmp")), key=natural_sort_key)

    if not image_files:
        print(f"경고: {scene_dir} 에서 .bmp 파일을 찾을 수 없습니다.")
        return

    print(f"총 {len(image_files)}장의 이미지를 처리합니다.")

    # YOLO 모델 로드
    model = YOLO("yolo11x.pt")

    for idx, img_path in enumerate(image_files, 1):
        print(f"[{idx}/{len(image_files)}] 검출 중: {img_path.name}")

        # YOLO 추론 (conf=0.25, 로그 출력 최소화)
        results = model(str(img_path), conf=0.25, verbose=False)

        detected_coords = []
        for result in results:
            boxes = result.boxes
            for box in boxes:
                # 좌상단(x1, y1), 우하단(x2, y2)
                x1, y1, x2, y2 = map(int, box.xyxy[0].tolist())

                # 4개 꼭짓점 매핑 (ul, ur, dl, dr) -> x1 y1 x2 y1 x1 y2 x2 y2
                coord_string = f"{x1} {y1} {x2} {y1} {x1} {y2} {x2} {y2}"
                detected_coords.append(coord_string)

        # 출력 텍스트 파일명: scene_01-n_box.txt
        output_txt_path = output_dir / f"{img_path.stem}_box.txt"

        # 좌표 데이터 저장
        with open(output_txt_path, "w", encoding="utf-8") as f:
            f.write(f"{len(detected_coords)}\n")
            for coord in detected_coords:
                f.write(f"{coord}\n")

        print(
            f"  -> 검출된 객체: {len(detected_coords)}개 | 저장 경로: {output_txt_path.name}"
        )


if __name__ == "__main__":
    batch_detect_and_save_boxes()