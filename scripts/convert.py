import os
from pathlib import Path
from PIL import Image


def convert_jpg_to_24bit_bmp():
    # 기준 경로 설정 (scripts/ 폴더의 상위 디렉토리)
    base_dir = Path(__file__).resolve().parent.parent

    # 입력 및 출력 디렉토리 경로 설정
    input_dir = base_dir / "input" / "scene_01"
    output_dir = base_dir / "input" / "scene"

    # 출력 디렉토리가 없으면 자동 생성
    output_dir.mkdir(parents=True, exist_ok=True)

    # 지원할 이미지 확장자 목록 (.jpg, .jpeg)
    image_extensions = ["*.jpg", "*.jpeg", "*.JPG", "*.JPEG"]

    image_files = []
    for ext in image_extensions:
        image_files.extend(input_dir.glob(ext))

    if not image_files:
        print(f"경고: {input_dir} 에서 이미지 파일(.jpg)을 찾을 수 없습니다.")
        return

    for img_path in image_files:
        print(f"변환 중: {img_path.name}")

        try:
            # 이미지 열기
            with Image.open(img_path) as img:
                # 24비트 BMP 생성을 위해 RGB 모드로 변환 (알파 채널 제거 및 24비트 보장)
                rgb_img = img.convert("RGB")

                # 출력 파일명 생성 (확장자를 .bmp로 변경)
                output_file_path = output_dir / f"{img_path.stem}.bmp"

                # BMP 형식으로 저장
                rgb_img.save(output_file_path, format="BMP")
                print(f"  -> 저장 완료: {output_file_path.name}")

        except Exception as e:
            print(f"  -> 변환 실패 ({img_path.name}): {e}")


if __name__ == "__main__":
    convert_jpg_to_24bit_bmp()