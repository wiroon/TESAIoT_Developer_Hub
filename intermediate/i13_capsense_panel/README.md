# แผง CapSense (CapSense Panel)

ตัวอย่างนี้อ่านปุ่มสัมผัส CapSense และ linear slider บนบอร์ด Eva Kit และแสดงสถานะบนหน้าจอ ตัวบ่งชี้ LED 2 ตัวแสดงสถานะปุ่ม (เปิด/ปิด) และ bar widget สะท้อนตำแหน่ง slider (0-100) ตัวอย่างทั้งหมดถูกครอบด้วย `#if BSP_HAS_CAPSENSE`

ปุ่ม CapSense 2 ตัวแสดงเป็น LVGL LED widget ซึ่งเป็นตัวบ่งชี้วงกลมเปิด/ปิดพร้อมการเปลี่ยนสี เมื่อสัมผัสปุ่ม LED ที่ตรงกันจะเปลี่ยนเป็นสีเขียว เมื่อปล่อยจะเปลี่ยนเป็นสีเทาเข้ม label ใต้ LED แต่ละตัวระบุว่าเป็น BTN0 และ BTN1

CapSense linear slider ถูก map ไปยัง bar widget แนวนอนที่กว้างเต็มแผง ค่า slider (0-100) กำหนดระดับการเติมของ bar โดยตรง label ตัวเลขข้าง bar แสดงเปอร์เซ็นต์ปัจจุบัน

timer 30ms สำรวจฮาร์ดแวร์ CapSense สำหรับการอัปเดตสถานะปุ่มและ slider อัตราการสำรวจที่เร็วนี้ทำให้การตอบสนองสัมผัสรวดเร็ว อุปกรณ์ต่อพ่วง CapSense มีเฉพาะใน Eva Kit ดังนั้น AI Kit และ Game Console build จะแสดงข้อความ "not available"

รองรับ TESAIoT Dev Kit และ PSoC Edge Eva Kit
