# Generated by Django 5.1.2 on 2024-10-31 12:58

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('core', '0001_initial'),
    ]

    operations = [
        migrations.RenameModel(
            old_name='DriverTickets',
            new_name='DriverTicket',
        ),
        migrations.RenameModel(
            old_name='Scans',
            new_name='Scan',
        ),
    ]