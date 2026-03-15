# ข้อมูล Accelerometer แบบเรียลไทม์ (Real-Time Accelerometer)

ตัวอย่างนี้รวมกราฟเส้นสดกับ label แสดงค่าตัวเลข 3 ตัว เพื่อแสดงข้อมูล BMI270 accelerometer แบบเรียลไทม์ กราฟอัปเดตทุก 50ms (20 Hz) ให้ผลตอบรับภาพที่ลื่นไหลของการเคลื่อนไหวบอร์ดทั้ง 3 แกน

เซ็นเซอร์ BMI270 ถูกเริ่มต้นเมื่อบูตและอ่านภายใน LVGL timer callback ค่า accelerometer ดิบถูกแปลงเป็น milli-g และส่งเข้า 3 chart series (X=แดง, Y=เขียว, Z=น้ำเงิน) กราฟเก็บข้อมูล 100 จุดต่อ series โดยช่วงแกน Y อยู่ที่ -2000 ถึง +2000 mg

label ค่า 3 ตัววางเรียงแนวนอนใต้กราฟ แต่ละ label มีสีตรงกับ chart series แสดงค่าปัจจุบันในหน่วย milli-g การแสดงผลแบบคู่ (กราฟิก + ตัวเลข) นี้เป็นรูปแบบมาตรฐานที่ใช้ทั่วทุกหน้า sensor dashboard ใน firmware

สไตล์กราฟเป็นไปตาม BENTO dark theme: สีพื้นหลัง 0x142240, มุมโค้งมน และขอบบาง จุดบนเส้นถูกปิดด้วย `lv_obj_set_style_line_width(chart, 0, LV_PART_ITEMS)` เพื่อรูปลักษณ์เส้นที่สะอาด

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
