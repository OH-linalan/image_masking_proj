import json
import os
from pathlib import Path


def process_camera_data():
    # 기준 경로 설정 (현재 스크립트의 상위 디렉토리 기준)
    base_dir = Path(__file__).resolve().parent.parent

    # 입력 디렉토리 및 출력 디렉토리 경로
    input_dir = base_dir / "input" / "scene_01"
    output_dir = base_dir / "input" / "data"

    # 출력 디렉토리가 없으면 생성
    output_dir.mkdir(parents=True, exist_ok=True)

    # input/scene_01/ 내의 모든 json 파일 처리
    json_files = list(input_dir.glob("*.json"))

    if not json_files:
        print(f"경고: {input_dir} 에서 JSON 파일을 찾을 수 없습니다.")
        return

    for json_file in json_files:
        print(f"파일 처리 중: {json_file.name}")

        with open(json_file, "r", encoding="utf-8") as f:
            data = json.load(f)

        # JSON 구조에 따라 'cameras' 키 내부 혹은 최상위에 씬 정보가 있는 경우 모두 대응
        cameras = data.get("cameras", data)

        for scene_key, scene_info in cameras.items():
            if not isinstance(scene_info, dict):
                continue

            # K, R, t 값 추출
            k_list = scene_info.get("K", [])
            r_list = scene_info.get("R", [])
            t_list = scene_info.get("t", [])

            # 값이 정상적으로 존재하는 경우에만 저장
            if k_list and r_list and t_list:
                # K -> R -> t 순서대로 결합
                combined_values = k_list + r_list + t_list

                # txt 파일 저장 경로 설정 (예: input/data/scene_01-2.txt)
                output_file_path = output_dir / f"{scene_key}.txt"

                # 한 줄에 하나씩 값 저장
                with open(output_file_path, "w", encoding="utf-8") as out_f:
                    for val in combined_values:
                        out_f.write(f"{val}\n")

                print(f"  -> 저장 완료: {output_file_path.name}")


if __name__ == "__main__":
    process_camera_data()