# เลย์เอาต์ Grid (Grid Layout)

ตัวอย่างนี้สาธิตระบบ grid layout ของ LVGL โดยสร้าง grid ขนาด 2x2 แต่ละเซลล์มี Virtual LED สวิตช์สำหรับเปิดปิด และ label แสดงชื่อช่อง จำลองแผงควบคุม 4 ช่อง

คอลัมน์และแถวของ grid ถูกกำหนดด้วย descriptor array ผ่าน `lv_obj_set_grid_dsc_array` แต่ละ child ถูกวางในเซลล์เฉพาะด้วย `lv_obj_set_grid_cell` พร้อมดัชนีแถวและคอลัมน์ grid layout ให้การวางตำแหน่งแบบสองมิติที่แม่นยำ

สวิตช์ในแต่ละเซลล์ควบคุม LED ในเซลล์เดียวกันโดยใช้รูปแบบ user data pointer แสดงวิธีสร้างกลุ่มควบคุมอิสระหลายกลุ่มภายในเลย์เอาต์เดียว

grid layout มีประสิทธิภาพสำหรับการออกแบบ dashboard ที่ widget ต้องจัดตำแหน่งแถว/คอลัมน์อย่างแม่นยำ รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
