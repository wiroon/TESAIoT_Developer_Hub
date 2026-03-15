# ตัวแยกวิเคราะห์ USB Joystick HID (USB Joystick HID Parser)

ตัวอย่างนี้สาธิตการแยกวิเคราะห์ HID report จาก Logitech F310 gamepad ในโหมด DirectInput โดยใช้ SEGGER emUSB-Host stack ตัวแยกวิเคราะห์ดึงค่าแกนอะนาล็อก (X, Y, Z, Rz), ปุ่มดิจิทัล 12 ปุ่ม และ D-pad hat switch จากไบต์ HID report ดิบ จากนั้นส่งข้อมูลไปยัง application core ผ่าน IPC

F310 ในโหมด DirectInput (VID: 046D, PID: C216) ส่ง HID report ขนาด 8 ไบต์ผ่าน USB interrupt endpoint เลย์เอาต์ report แมป byte offset เฉพาะกับตัวควบคุม: ไบต์ 0-1 สำหรับแกน X/Y ของสติ๊กซ้าย, ไบต์ 2-3 สำหรับแกน Z/Rz ของสติ๊กขวา, ไบต์ 4 สำหรับ D-pad hat (0-7 สำหรับ 8 ทิศทาง, 0x0F สำหรับกลาง) และไบต์ 5-6 สำหรับบิตปุ่ม 12 ปุ่ม

IPC bridge ใช้คำสั่ง 2 ตัว: `IPC_CMD_JOYSTICK_INIT` (0xC1) เพื่อส่งสัญญาณให้ joystick task เริ่ม polling USB และ `IPC_CMD_JOYSTICK_STATE` (0xC0) เพื่อส่งสถานะ joystick ปัจจุบันจาก CM55 core (ซึ่งรัน USB stack) ไปยัง CM33_NS application core สถานะถูกบรรจุในข้อความ IPC ขนาด 8 ไบต์

UI แสดงการแสดงผลแบบ real-time ของอินพุต joystick ทั้งหมด ได้แก่ ค่าแกนเป็น progress bar, สถานะปุ่มเป็นตัวบ่งชี้สี และทิศทาง D-pad เป็นลูกศรเข็มทิศ สถานะ USB host enumeration และข้อมูล device descriptor แสดงในแผงข้อมูล

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit (ทั้งสองบอร์ดรองรับ USB host บน CM55)
