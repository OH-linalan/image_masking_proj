### Prerequisites
- GCC / G++ Compiler (C++17 이상 지원 및 MinGW-w64 환경)
- Make
- Git
- vcpkg
- conda
### vcpkg 설치 및 C++ 의존성 패키지 설치
```Bash
# 프로젝트 외부 C:, D: 등에 위치시키는 것을 권장
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Windows (PowerShell):
.\bootstrap-vcpkg.bat
# Linux/macOS 또는 Git Bash:
./bootstrap-vcpkg.sh

# 환경변수
# Bash / Git Bash:
export VCPKG_ROOT=/path/to/vcpkg
# Windows (PowerShell):
$env:VCPKG_ROOT="경로"

cd 경로
$VCPKG_ROOT/vcpkg install --triplet x64-mingw-dynamic

```

### C++ 프로젝트 빌드 및 실행
```Bash
# 전체 프로젝트 빌드
make
# 빌드 후 실행
make run
# 빌드 출력물 삭제
make clean
```

### Conda 가상환경 설정
```Bash
conda env create -f environment.yml
conda activate cv_env
```