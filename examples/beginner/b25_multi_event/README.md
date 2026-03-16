# บันทึก Event หลายประเภท (Multi Event Logger)

ตัวอย่างนี้ลงทะเบียน callback เดียวสำหรับ `LV_EVENT_ALL` บนปุ่ม จากนั้นใช้ switch-case เพื่อระบุและบันทึกแต่ละประเภท event label ที่เลื่อนได้แสดงประวัติ event แสดงลำดับของ event ที่เกิดระหว่างการโต้ตอบของผู้ใช้

เมื่อผู้ใช้สัมผัส ค้าง และปล่อยปุ่ม callback จะจับทุก event ตามลำดับ: PRESSED, PRESSING, RELEASED, CLICKED และอื่นๆ เปิดเผยวงจรชีวิต event ทั้งหมดที่ LVGL สร้างสำหรับแต่ละการสัมผัส

บันทึก event ถูกเก็บใน static buffer ที่เลื่อนแสดง event ล่าสุด ปุ่ม clear รีเซ็ตบันทึก นี่เป็นเครื่องมือ debug ที่มีค่าอย่างยิ่งเมื่อสร้างการโต้ตอบแบบกำหนดเอง

การเข้าใจลำดับ event สำคัญต่อการพัฒนา UI ขั้นสูง รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
