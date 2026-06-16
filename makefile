# 컴파일러 설정
CXX = g++

# 컴파일 옵션 (디버깅 정보 포함 및 모든 경고 출력, C++17 표준 지정)
CXXFLAGS = -Wall -g -std=c++17

# 💡 vcpkg 기반 OpenCV x64-mingw-dynamic 경로 설정
VCPKG_ROOT = $(CURDIR)/vcpkg_installed/x64-mingw-dynamic

# 1. 헤더 파일 디렉터리 추가 (opencv4 서브폴더까지 명시)
INC_FLAGS = -I$(VCPKG_ROOT)/include/opencv4

# 2. 라이브러리 디렉터리 추가
LDFLAGS = -L$(VCPKG_ROOT)/lib

# 3. 링크할 OpenCV 모듈 지정 (-l)
LDLIBS = -lopencv_core4 \
         -lopencv_imgcodecs4 \
         -lopencv_imgproc4 \
         -lopencv_features2d4 \
         -lopencv_calib3d4

# 타겟 이름 및 경로 (본체 실행 파일)
TARGET = output/a.out

# 소스 파일
SRCS = main.cpp

# 기본 빌드 규칙 (make 또는 make all)
all: $(TARGET)

# 💡 본체 실행 파일 생성 규칙
$(TARGET): $(SRCS)
	@mkdir -p output
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS) $(LDLIBS)
	@cp $(VCPKG_ROOT)/bin/*.dll output/

# 빌드 결과물 삭제 규칙
clean:
	rm -rf output

# 💡 빌드 및 본체 실행 규칙 (편의용: make run)
run: all
	./$(TARGET)

# 테스트 파일 전용 빌드 및 실행 규칙 (make test)
test:
	@mkdir -p output
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -o output/test_cv.out test.cpp $(LDFLAGS) $(LDLIBS)
	@cp $(VCPKG_ROOT)/bin/*.dll output/
	./output/test_cv.out