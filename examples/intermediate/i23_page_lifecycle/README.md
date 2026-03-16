# วงจรชีวิตของหน้า (Page Lifecycle)

ตัวอย่างนี้สาธิตวงจรชีวิตที่สมบูรณ์ของหน้าในระบบ page manager ของ BENTO หน้ามี callback 3 ตัว: `create` (เรียกครั้งเดียวเมื่อนำทางไปยังหน้า) `render` (เรียกทุกรอบ frame tick) และ `destroy` (เรียกเมื่อนำทางออกจากหน้า)

โครงสร้าง `page_def_t` เก็บ function pointer สำหรับทั้ง 3 เฟสของวงจรชีวิต ลงทะเบียนกับ page manager ผ่าน `pm_register(&s_pm, PAGE_ID, &def)` เมื่อ page manager เปิดใช้งานหน้านี้ มันจะเรียก `create` เพื่อสร้าง widget tree จากนั้นเรียก `render` ซ้ำๆ สำหรับอัปเดตข้อมูล และสุดท้ายเรียก `destroy` เพื่อทำความสะอาด

callback `create` สร้าง UI (label, chart, container) callback `render` อัปเดตข้อมูลไดนามิก (ค่าเซ็นเซอร์, timestamp) callback `destroy` ปล่อยทรัพยากรที่จัดสรรและหยุด timer การจัดการ destroy อย่างถูกต้องป้องกัน memory leak เมื่อนำทางระหว่างหน้า

ตัวอย่างนี้บันทึกทุก lifecycle event ไปยัง label ที่มองเห็นได้ แสดงลำดับและจังหวะเวลาที่แม่นยำของการเรียก create/render/destroy ตัวนับติดตามเฟรม render และ timestamp แสดงเวลาที่แต่ละเฟสถูกทริกเกอร์

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
