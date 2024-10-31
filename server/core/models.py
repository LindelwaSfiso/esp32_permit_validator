from django.db import models
from django.utils import timezone


class Driver(models.Model):
    """
    Models a driver. Each driver has a dedicated barcode that uniquely identifies him/her.
    """
    first_name = models.CharField(max_length=50)
    last_name = models.CharField(max_length=50)
    gender = models.IntegerField(choices=((1, "Male"), (2, "Female")))
    region = models.IntegerField(choices=((1, "Manzini"), (2, "Hhohho"), (3, "Shiselweni"), (4, "Lubombo")), default=1)
    licence_number = models.CharField(max_length=50)
    barcode = models.CharField(max_length=100)
    valid_up_to = models.DateTimeField()

    def is_permit_valid(self):
        return timezone.now() >= self.valid_up_to


class DriverTicket(models.Model):
    driver = models.ForeignKey(Driver, related_name="tickets", on_delete=models.CASCADE)
    amount = models.DecimalField(decimal_places=2, max_digits=10)
    status = models.IntegerField(choices=((1, "Unpaid"), (2, "Paid")), default=1)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)


class Scan(models.Model):
    """
    Models saved barcode scans. Note: the operate can also use a keypad dail to input the barcode should the scan not
    work.
    """
    driver = models.ForeignKey(Driver, related_name="scans", on_delete=models.CASCADE)
    saved_scan = models.ImageField(upload_to="scans/", null=True)
    is_keypad = models.BooleanField(default=False)
    created_at = models.DateTimeField(auto_now_add=True)
