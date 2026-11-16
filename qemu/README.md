# QEMU에 자체 I2C 컨트롤러 모델 추가하기 (방법 3 예제)

우리 SoC의 I2C 컨트롤러를 QEMU 안에서 "진짜 하드웨어처럼" 동작시키는 예제입니다.
여기서는 **예시용으로 임의로 정의한** 컨트롤러 `jm-i2c`를 만들었고, 실제 SoC가 정해지면
레지스터 표와 동작만 그 칩의 레퍼런스 매뉴얼대로 바꾸면 됩니다.

## 파일
| 파일 | 역할 |
|---|---|
| `model/jm_i2c.c`, `model/jm_i2c.h` | QEMU 안에서 도는 **컨트롤러 모델** (C) |
| `qemu-8.2.2-jm-i2c.patch` | 모델을 QEMU에 연결하는 변경 (Kconfig, meson, mps2 머신) |
| `demo/i2c_jm.c` | 펌웨어 쪽 **드라이버**: 부트로더의 `hal_i2c_xfer()`를 이 컨트롤러로 구현 |
| `demo/demo.c` | 드라이버를 시험하는 베어메탈 프로그램 (PASS/FAIL 출력) |

## 전체 그림
```
 펌웨어(ARM)                         QEMU (PC에서 실행)
 demo.c ─ hal_i2c_xfer()  ──MMIO──▶  jm_i2c.c  ──QEMU I2C API──▶  at24c-eeprom (주소 0x50)
          (i2c_jm.c)      레지스터    컨트롤러 모델   i2c_start/send/recv   (QEMU 내장 EEPROM 모델)
```
펌웨어가 `0x40030000`의 레지스터에 쓰면 QEMU가 모델의 `jm_i2c_write()`를 호출하고,
모델은 그것을 QEMU의 I2C 버스 호출로 바꿔 버스에 붙은 장치(EEPROM)에 전달합니다.

## 컨트롤러 레지스터 (예시)
| 오프셋 | 이름 | 설명 |
|---|---|---|
| 0x00 | CTRL | bit0 = 사용 |
| 0x04 | ADDR | 7비트 타깃 주소 |
| 0x08 | CMD | 1 START_W, 2 START_R, 3 WRITE(TXDATA), 4 READ+ACK, 5 READ+NACK(마지막 바이트), 6 STOP |
| 0x0C | TXDATA | 보낼 바이트 |
| 0x10 | RXDATA | 받은 바이트 |
| 0x14 | STATUS | bit1 DONE, bit2 NACK, bit3 ERR (NACK/ERR은 1을 써서 지움) |
| 0x18 | ID | `0x4A4D4932` |

## 모델 코드가 하는 일 (`model/jm_i2c.c`)
1. **레지스터 접근:** `jm_i2c_read`/`jm_i2c_write`가 MMIO 읽기/쓰기를 받습니다.
2. **명령 해석:** CMD 레지스터에 쓰면 `jm_i2c_command()`가 QEMU I2C API로 바꿉니다.
   - `START_*` → `i2c_start_transfer()` (응답하는 장치가 없으면 실패 → STATUS.NACK)
   - `WRITE` → `i2c_send()` (장치가 NACK하면 실패)
   - `READ_*` → `i2c_recv()`, 마지막 바이트는 `i2c_nack()`
   - `STOP` → `i2c_end_transfer()`
3. **등록:** `TypeInfo`로 장치 타입을 등록하고, `realize`에서 MMIO 영역과 `i2c` 자식 버스를 만듭니다.
   `reset`과 `vmstate`(저장/복원)도 갖춥니다.

현재 모델은 명령이 즉시 끝납니다. 실제 컨트롤러처럼 지연이나 인터럽트를 넣으려면
타이머(`QEMUTimer`)와 IRQ(`sysbus_init_irq`/`qemu_set_irq`)를 추가하면 됩니다.

## 실행 순서

### 1. QEMU 소스 준비 (QEMU 8.2.2 기준)
```bash
mkdir qemu-build && cd qemu-build
# 방법 A: 공식 tarball (download.qemu.org 접근이 되는 환경)
#   wget https://download.qemu.org/qemu-8.2.2.tar.xz && tar xf qemu-8.2.2.tar.xz && mv qemu-8.2.2 src
# 방법 B: Ubuntu 패키지 소스 (deb-src를 켠 뒤 apt-get source --download-only qemu)
mkdir src && tar -xf qemu_8.2.2+ds.orig.tar.xz -C src --strip-components=1
```

### 2. 모델 복사 + 패치 적용
```bash
cp <repo>/qemu/model/jm_i2c.h src/include/hw/i2c/
cp <repo>/qemu/model/jm_i2c.c src/hw/i2c/
cd src && patch -p1 < <repo>/qemu/qemu-8.2.2-jm-i2c.patch && cd ..
```
패치는 (a) `hw/i2c`에 `JM_I2C` 빌드 항목 추가, (b) `mps2-an500` 머신의 `0x40030000`에
컨트롤러를 만들고 그 버스에 AT24C EEPROM(주소 0x50, 256바이트)을 붙이는 것입니다.

### 3. 빌드 (ARM 에뮬레이터만, 수 분~수십 분)
```bash
sudo apt-get install -y meson ninja-build libglib2.0-dev libpixman-1-dev
mkdir build && cd build
../src/configure --target-list=arm-softmmu --disable-docs --disable-tools --without-default-features \
                 --disable-install-blobs
ninja qemu-system-arm
```
Ubuntu의 `+ds` 소스는 일부 펌웨어 파일이 빠져 있어서, 이 경우 `configure`가 `edk2-*.fd.bz2`/`bios.bin` 등이
없다고 실패합니다. ARM 빌드에는 필요 없으므로 `src/pc-bios/meson.build`의 `if unpack_edk2_blobs`를
`if false`로 바꾸고, `blobs` 목록의 파일들을 빈 파일로 만들어 두면 통과합니다(공식 tarball에서는 불필요).

### 4. 데모 실행
```bash
cd <repo>/qemu/demo
make run QEMU=<경로>/build/qemu-system-arm
```
기대 출력:
```
jm-i2c demo
PASS  controller ID is JMI2
PASS  probe 0x50 (EEPROM) ACKs
PASS  probe 0x51 (nothing) NACKs
PASS  write 4 bytes @0x10
PASS  read back with repeated START
...
RESULT: PASS
```
종료 코드는 실패 개수입니다(0이면 성공). **모델이 없는 순정 QEMU에서는 FAIL(종료 코드 7)**이 나옵니다.

## 우리 SoC에 맞게 바꾸려면
1. `jm_i2c.c`의 레지스터 정의/동작을 레퍼런스 매뉴얼에 맞춰 교체 (FIFO, 클럭 분주, 인터럽트 포함).
2. 머신 코드(`hw/arm/`)에서 SoC의 실제 주소에 배치하고, 필요한 만큼 버스/장치를 붙입니다.
   다른 장치는 코드 대신 명령줄로도 붙일 수 있습니다: `-device <타입>,bus=<버스 id>,address=0x..`
3. `demo/i2c_jm.c`를 그 컨트롤러의 실제 드라이버(`platform/<soc>/`의 `hal_i2c_xfer`)로 교체합니다.
   이 드라이버가 QEMU에서 통과하면 같은 코드를 실제 보드에서 쓰게 됩니다.

## 한계
- 이 모델은 예시 컨트롤러입니다. 실제 칩의 컨트롤러 동작(클럭 스트레칭, 타이밍, FIFO 깊이, 에러 조건)은
  그 칩의 매뉴얼 기준으로 모델을 쓰기 전까지 검증되지 않습니다.
- 현재 `platform/`의 부트로더 본체는 이 머신에서 실행되지 않습니다(QSPI/Boot ROM 모델이 없음). 이 예제는 I2C
  드라이버와 모델만 단독으로 시험합니다.
- 빌드 결과물(`qemu-system-arm`)은 저장소에 포함하지 않았습니다.
- I3C 모델은 같은 방식으로 만들 수 있지만 이 예제에는 없습니다. I3C는 동적 주소 할당(ENTDAA)과 IBI 때문에
  모델 코드가 I2C보다 큽니다.
