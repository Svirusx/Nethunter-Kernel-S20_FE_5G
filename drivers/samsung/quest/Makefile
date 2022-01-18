obj-$(CONFIG_SEC_QUEST)	+= sec_quest.o \
							sec_quest_param.o					
							
obj-$(CONFIG_SEC_QUEST_BPS_CLASSIFIER)	+= sec_quest_bps.o

ifeq ($(CONFIG_SEC_FACTORY), y)
obj-$(CONFIG_SEC_QUEST)	+= sec_quest_qdaf.o
endif