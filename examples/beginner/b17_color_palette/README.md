# จานสี (Color Palette)

ตัวอย่างนี้แสดงสี Palette ของ LVGL จำนวน 12 สีใน grid ขนาด 4x3 เป็นข้อมูลอ้างอิงภาพ แต่ละเซลล์แสดงสี่เหลี่ยมสีพร้อมชื่อ Palette ด้านล่าง ทำให้ง่ายต่อการระบุสีที่ใช้ได้

grid ใช้ `LV_LAYOUT_GRID` พร้อม column และ row descriptor array แต่ละเซลล์เป็น container ที่มีสีพื้นหลังตั้งเป็นค่า `lv_palette_main()` ที่เกี่ยวข้อง ชื่อ Palette แสดงเป็น label สีขาวบนพื้นสีจัดกลางในแต่ละเซลล์

LVGL มีสี Palette ในตัว 16 สี แต่ละสีมีรุ่น lighten และ darken grid อ้างอิงนี้แสดงเฉดหลักของ 12 Palette ที่ใช้มากที่สุด: Red, Pink, Purple, Deep Purple, Indigo, Blue, Cyan, Teal, Green, Lime, Yellow และ Orange

ตัวอย่างนี้เป็นข้อมูลอ้างอิงที่มีประโยชน์สำหรับเลือกสีเมื่อออกแบบ UI ด้วย LVGL รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
