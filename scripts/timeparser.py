import os
import re

# 로그가 담긴 입력 파일 경로 및 결과 저장 경로 설정
INPUT_LOG_PATH = "log.txt"  # 터미널 출력을 저장한 로그 파일명
OUTPUT_TXT_PATH = "output/timelog.txt"

# 정규표현식 패턴 정의
# [header: operation] Execution time: X us= Y ms 형태를 매칭
pattern = re.compile(
    r"\[(?P<header>[^:]+):\s*(?P<operation>[^\]]+)\]\s*Execution time:\s*(?P<us>\d+)\s*us="
)


def parse_time_log():
    # output 디렉터리가 없으면 자동 생성
    os.makedirs(os.path.dirname(OUTPUT_TXT_PATH), exist_ok=True)

    parsed_lines = []

    # 로그 파일 읽기 (UTF-8 지정)
    try:
        with open(INPUT_LOG_PATH, "r", encoding="utf-8") as f:
            for line in f:
                match = pattern.search(line)
                if match:
                    header = match.group("header").strip()
                    operation = match.group("operation").strip()
                    us_value = match.group("us").strip()

                    # 원하는 형식으로 구성: header, operation, us
                    formatted_line = f"{header}, {operation}, {us_value}"
                    parsed_lines.append(formatted_line)

        # 결과 파일 파싱하여 쓰기
        with open(OUTPUT_TXT_PATH, "w", encoding="utf-8") as f:
            for item in parsed_lines:
                f.write(item + "\n")

        print(
            f"Successfully parsed {len(parsed_lines)} log entries -> {OUTPUT_TXT_PATH}"
        )

    except FileNotFoundError:
        print(
            f"Error: {INPUT_LOG_PATH} 파일을 찾을 수 없습니다. 로그 파일 경로를 확인해 주세요."
        )


if __name__ == "__main__":
    parse_time_log()