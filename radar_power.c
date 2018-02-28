#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/kdev_t.h>

#define FRONT_PWR_EN    145
#define BACK_PWR_EN     146
int front = 1;
int back = 1;

static ssize_t front_show(struct device *d, struct device_attribute *attr, char *buf)
{
    front = gpio_get_value(FRONT_PWR_EN);
    return sprintf(buf, "%d\n", !!front);
}

static ssize_t front_store(struct device *d, struct device_attribute *attr,const char *buf,size_t count) 
{
    int ret;
    u8 v;

    ret = kstrtou8(buf, 0, &v);
    if (ret < 0)
        return ret;

    gpio_direction_output(FRONT_PWR_EN, !!v);

    return count;
}

static ssize_t back_show(struct device *d, struct device_attribute *attr, char *buf)
{
    back = gpio_get_value(BACK_PWR_EN);
    return sprintf(buf, "%d\n", !!back);
}

static ssize_t back_store(struct device *d, struct device_attribute *attr,const char *buf,size_t count) 
{
    int ret;
    u8 v;

    ret = kstrtou8(buf, 0, &v);
    if (ret < 0)
        return ret;

    gpio_direction_output(BACK_PWR_EN, !!v);

    return count;
}

/*
在某个kobject下创建属性文件使用：
static DEVICE_ATTR(front, S_IWUSR | S_IRUSR | S_IROTH, front_show, front_store);
static DEVICE_ATTR(back, S_IWUSR | S_IRUSR | S_IROTH, back_show, back_store);

static struct attribute *radar_attrs[] = {
    &dev_attr_front.attr,
    &dev_attr_back.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = radar_attrs,
};

sysfs_create_group(&kobj, &attr_group);
*/

/*
class目录下创建属性文件可以这样写：
static struct class_attribute radar_class_attrs[] = {
    __ATTR(front, 0660, front_show, front_store),
    __ATTR(back, 0660, back_show, back_store),
    __ATTR_NULL,
};
*/

/* 另一种写法 */
static CLASS_ATTR(front, S_IRUGO | S_IWUSR, front_show, front_store);
static CLASS_ATTR(back, S_IRUGO | S_IWUSR, back_show, back_store);
static struct class_attribute *radar_class_attrs[] = {
        &class_attr_front,
        &class_attr_back,
};

static int __init radar_power_init(void)
{
    int ret = 0;
    int i = 0;
    int attrs_num = 0;
    struct class* radar_class = NULL;
    struct class_attribute** radar_attrs = radar_class_attrs;

    pr_info("%s +\n", __func__);

    ret = gpio_request(FRONT_PWR_EN, "radar_front");
    if (ret < 0) {
        pr_err("FRONT_PWR_EN gpio request failed.\n");
        return -1;
    }

    ret = gpio_request(BACK_PWR_EN, "radar_back");
    if (ret < 0) {
        pr_err("BACK_PWR_EN gpio request failed.\n");
        return -1;
    }

    gpio_direction_output(FRONT_PWR_EN, 1);
    gpio_direction_output(BACK_PWR_EN, 1);


    radar_class = class_create(THIS_MODULE, "radar");
    if (IS_ERR(radar_class)) {
        pr_err("radar class create failed.\n");
        return PTR_ERR(radar_class);
    }

    attrs_num = ARRAY_SIZE(radar_class_attrs);
    for(i = 0; i < attrs_num; i++)
    {
        ret = class_create_file(radar_class, radar_attrs[i]);
        if (ret) {
            (void)class_destroy(radar_class);
            pr_err("Failed to create file, %d", i);
            return ret;
        }
    }

    pr_info("%s -\n", __func__);

    return 0;
}

static void __exit radar_power_exit(void)
{
    pr_info("%s\n", __func__);
}

module_init(radar_power_init);
module_exit(radar_power_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Radar power control interface");
