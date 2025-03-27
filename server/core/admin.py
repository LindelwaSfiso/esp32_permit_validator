from django.contrib import admin
from core.models import Driver, DriverTicket, Scan


@admin.register(Driver)
class DriverAdmin(admin.ModelAdmin):
	list_per_page = 25
	list_display = ["id", "first_name", "last_name", "barcode", "is_permit_valid"]


@admin.register(DriverTicket)
class DriverTicketsAdmin(admin.ModelAdmin):
	list_per_page = 25
	list_display = ["driver", "amount", "get_status_display", "created_at"]


@admin.register(Scan)
class ScansAdmin(admin.ModelAdmin):
	list_per_page = 25
	list_display = ["driver", 'is_valid', 'valid_up_to', "is_keypad", "saved_scan"]

	def is_valid(self, scan):
		return scan.driver.is_permit_valid()

	def valid_up_to(self, scan):
		return scan.driver.valid_up_to


admin.site.site_header = 'Police: Permit monitoring system'
admin.site.site_title = 'Drivers permit system'
admin.site.index_title = 'Permit monitoring Administration'
